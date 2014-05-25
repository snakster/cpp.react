
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "tbb/concurrent_vector.h"
#include "tbb/queuing_mutex.h"

#include "react/common/Concurrency.h"
#include "react/common/Types.h"
#include "react/detail/ReactiveInput.h"
#include "react/detail/IReactiveNode.h"
#include "react/detail/IReactiveEngine.h"
#include "react/detail/ObserverBase.h"
#include "react/detail/Options.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TurnBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <bool is_thread_safe>
class TurnBase
{
public:
    TurnBase(TurnIdT id, TurnFlagsT flags);

    TurnIdT Id() const;

    void QueueForDetach(IObserver& obs);

    template <typename D>
    friend class InputManager;

    template <typename D>
    friend class ContinuationHolder;

private:
    using ContinuationT =   ContinuationInput<is_thread_safe>;

    template <typename D>
    void detachObservers();
};

// Not thread-safe
template <>
class TurnBase<false>
{
public:
    TurnBase(TurnIdT id, TurnFlagsT flags) :
        id_{ id }
    {}

    TurnIdT Id() const { return id_; }

    void QueueForDetach(IObserver& obs)
    {
        detachedObservers_.push_back(&obs);
    }

    template <typename D>
    friend class InputManager;

    template <typename D>
    friend class ContinuationHolder;

private:
    using ObsVectT =        std::vector<IObserver*>;
    using ContinuationT =   ContinuationInput<false>;

    TurnIdT    id_;

    template <typename D>
    void detachObservers()
    {

        auto& registry = DomainSpecificObserverRegistry<D>::Instance();

        for (auto* o : detachedObservers_)
            registry.Unregister(o);

        detachedObservers_.clear();
    }

    ObsVectT        detachedObservers_;
    ContinuationT   continuation_;
};

// Thread-safe
template <>
class TurnBase<true>
{
public:
    TurnBase(TurnIdT id, TurnFlagsT flags) :
        id_{ id }
    {}

    TurnIdT Id() const { return id_; }

    void QueueForDetach(IObserver& obs)
    {
        // Allocation of concurrent vector is not cheap -> create on demand
        std::call_once(detachedObserversInit_, [this] {
            detachedObserversPtr_.reset(new ObsVectT());
        });

        detachedObserversPtr_->push_back(&obs);
    }

    template <typename D>
    friend class InputManager;

    template <typename D>
    friend class ContinuationHolder;

private:
    using ObsVectT = tbb::concurrent_vector<IObserver*>;
    using ContinuationT =  ContinuationInput<true>;

    TurnIdT    id_;

    template <typename D>
    void detachObservers()
    {
        if (detachedObserversPtr_ != nullptr)
        {
            auto& registry = DomainSpecificObserverRegistry<D>::Instance();

            for (auto* o : *detachedObserversPtr_)
                registry.Unregister(o);

            detachedObserversPtr_->clear();
        }
    }

    std::once_flag              detachedObserversInit_;
    std::unique_ptr<ObsVectT>   detachedObserversPtr_;
    ContinuationT               continuation_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TurnQueueManager
///////////////////////////////////////////////////////////////////////////////////////////////////
class TurnQueueManager
{
public:
    class QueueEntry
    {
    public:
        explicit QueueEntry(TurnFlagsT flags) :
            isMergeable_{ (flags & enable_input_merging) != 0 }
        {}

        inline void Append(QueueEntry& tr)
        {
            successor_ = &tr;
            tr.blockCondition_.Block();
        }

        inline void WaitForUnblock()
        {
            blockCondition_.WaitForUnblock();
        }

        inline void RunMergedInputs() const
        {
            for (const auto& e : merged_)
                e.first();
        }

        inline void UnblockSuccessors()
        {
            for (const auto& e : merged_)
                e.second->Unblock();

            if (successor_)
                successor_->blockCondition_.Unblock();
        }

        template <typename F>
        inline bool TryMerge(F&& inputFunc, BlockingCondition& caller)
        {
            if (!isMergeable_)
                return false;

            // Only merge if target is still blocked
            bool merged = blockCondition_.RunIfBlocked([&] {
                caller.Block();
                merged_.emplace_back(std::make_pair(std::forward<F>(inputFunc), &caller));
            });

            return merged;
        }

    private:
        using MergedDataVectT =
            std::vector<
                std::pair<
                    std::function<void()>,
                    BlockingCondition*>>;

        bool                isMergeable_;
        QueueEntry*         successor_ = nullptr;
        MergedDataVectT     merged_;
        BlockingCondition   blockCondition_;
    };

    template <typename F>
    inline bool TryMerge(F&& inputFunc)
    {
        bool merged = false;

        BlockingCondition caller;

        {// seqMutex_
            SeqMutexT::scoped_lock lock(seqMutex_);

            if (tail_)
                merged = tail_->TryMerge(std::forward<F>(inputFunc), caller);
        }// ~seqMutex_

        if (merged)
            caller.WaitForUnblock();

        return merged;
    }

    inline void StartTurn(QueueEntry& turn)
    {
        {// seqMutex_
            SeqMutexT::scoped_lock lock(seqMutex_);

            if (tail_)
                tail_->Append(turn);

            tail_ = &turn;
        }// ~seqMutex_

        turn.WaitForUnblock();
    }

    inline void EndTurn(QueueEntry& turn)
    {// seqMutex_
        SeqMutexT::scoped_lock lock(seqMutex_);

        turn.UnblockSuccessors();

        if (tail_ == &turn)
            tail_ = nullptr;
    }// ~seqMutex_

private:
    using SeqMutexT = tbb::queuing_mutex;

    SeqMutexT       seqMutex_;
    QueueEntry*     tail_ = nullptr;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DefaultQueueableTurn
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename TTurnBase
>
class DefaultQueueableTurn :
    public TTurnBase,
    public TurnQueueManager::QueueEntry
{
public:
    DefaultQueueableTurn(TurnIdT id, TurnFlagsT flags) :
        TTurnBase{ id, flags },
        TurnQueueManager::QueueEntry{ flags }
    {}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DefaultQueuingEngine
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    template <typename Turn_> class TTEngineBase,
    typename TTurnBase
>
class DefaultQueuingEngine : public TTEngineBase<DefaultQueueableTurn<TTurnBase>>
{
public:
    using TurnT = DefaultQueueableTurn<TTurnBase>;

    void OnTurnAdmissionStart(TurnT& turn)
    {
        queueManager_.StartTurn(turn);
    }

    void OnTurnAdmissionEnd(TurnT& turn)
    {
        turn.RunMergedInputs();
    }

    void OnTurnEnd(TurnT& turn)
    {
        queueManager_.EndTurn(turn);
    }

    template <typename F>
    bool TryMerge(F&& f)
    {
        return queueManager_.TryMerge(std::forward<F>(f));
    }

private:
    TurnQueueManager    queueManager_;
};

/****************************************/ REACT_IMPL_END /***************************************/
