#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include "tbb/concurrent_vector.h"
#include "tbb/queuing_mutex.h"
#include "tbb/spin_mutex.h"

#include "Observer.h"

#include "react/common/Types.h"
#include "react/logging/EventLog.h"
#include "react/logging/EventRecords.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

template <typename D, typename S>
class RSignal;

template <typename D, typename S>
class RVarSignal;

template <typename D, typename E>
class REvents;

template <typename D, typename E>
class REventSource;

enum class EventToken;

template
<
	typename D,
	typename TFunc,
	typename ... TArgs
>
inline auto MakeSignal(TFunc func, const RSignal<D,TArgs>& ... args)
	-> RSignal<D,decltype(func(args() ...))>;

////////////////////////////////////////////////////////////////////////////////////////
/// TransactionInput
////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurnInterface>
class TransactionInput
{
public:
	TransactionInput() :
		flags_{ 0 },
		isEmpty_{ true },
		admissionPrepareFunc_{ nullFunc },
		admissionCommitFunc_{ nullFunc },
		propagationFunc_{ nullFunc }
	{
	}

	void Reset()
	{
		flags_ = 0;
		isEmpty_ = true;
		admissionPrepareFunc_ = nullFunc;
		admissionCommitFunc_ = nullFunc;
		propagationFunc_ = nullFunc;

		concAdmissionPrepareBuf_.clear();
		concAdmissionCommitBuf_.clear();
		concPropagationBuf_.clear();
	}

	int		Flags() const			{ return flags_; }
	void	SetFlags(int flags)		{ flags_ = flags; }

	bool	IsEmpty() const	{ return isEmpty_; }

	template <typename R, typename V>
	void AddSignalInput(R& r, const V& v)
	{
		isEmpty_ = false;

		auto prevAdmissionPrepareFunc = admissionPrepareFunc_;
		admissionPrepareFunc_ = [prevAdmissionPrepareFunc, &r, v] (TTurnInterface& turn)
		{
			// First decide on new value for this transaction (last input wins)
			prevAdmissionPrepareFunc(turn);
			r.SetNewValue(v);
		};

		auto prevAdmissionCommitFunc = admissionCommitFunc_;
		admissionCommitFunc_ = [this, prevAdmissionCommitFunc, &r] (TTurnInterface& turn)
		{
			// Then apply all new values. If it's different to the old one.
			// Multiple inputs to the same node will call apply several times.
			// But only the first one returns true. Only for this one, a pulse event is scheduled.
			prevAdmissionCommitFunc(turn);

			if (r.ApplyNewValue())
			{
				R::Engine::OnInputNodeAdmission(r, turn);

				auto prevPropagationFunc = propagationFunc_;
				propagationFunc_ = [prevPropagationFunc, &r] (TTurnInterface& turn)
				{
					prevPropagationFunc(turn);
					R::Engine::OnNodePulse(r, turn);
				};
			}
		};
	}

	template <typename R, typename V>
	void AddEventInput(R& r, const V& v)
	{
		isEmpty_ = false;

		auto prevAdmissionCommitFunc = admissionCommitFunc_;
		admissionCommitFunc_ = [prevAdmissionCommitFunc, &r, v] (TTurnInterface& turn)
		{
			prevAdmissionCommitFunc(turn);
			r.SetCurrentTurn(turn);
			r.Push(v);

			if (! r.admissionFlag_.test_and_set())
			{
				r.propagationFlag_.clear();
				R::Engine::OnInputNodeAdmission(r, turn);
			}
		};
		
		auto prevPropagationFunc = propagationFunc_;
		propagationFunc_ = [prevPropagationFunc, &r, v] (TTurnInterface& turn)
		{
			prevPropagationFunc(turn);

			if (! r.propagationFlag_.test_and_set())
			{
				r.admissionFlag_.clear();
				R::Engine::OnNodePulse(r, turn);
			}
		};
	}

	template <typename R, typename V>
	void AddSignalInput_Safe(R& r, const V& v)
	{
		isEmpty_ = false;

		concAdmissionPrepareBuf_.push_back([&r, v] (TTurnInterface& turn)
		{
			r.SetNewValue(v);
		});

		concAdmissionCommitBuf_.push_back([this, &r] (TTurnInterface& turn)
		{
			if (r.ApplyNewValue())
			{
				R::Engine::OnInputNodeAdmission(r, turn);

				auto prevPropagationFunc = propagationFunc_;
				propagationFunc_ = [prevPropagationFunc, &r] (TTurnInterface& turn)
				{
					prevPropagationFunc(turn);
					R::Engine::OnNodePulse(r, turn);
				};
			}
		});
	}

	template <typename R, typename V>
	void AddEventInput_Safe(R& r, const V& v)
	{
		isEmpty_ = false;

		concAdmissionCommitBuf_.push_back([&r, v] (TTurnInterface& turn)
		{
			r.SetCurrentTurn(turn);
			r.Push(v);

			if (! r.admissionFlag_.test_and_set())
			{
				r.propagationFlag_.clear();
				R::Engine::OnInputNodeAdmission(r, turn);
			}
		});

		concPropagationBuf_.push_back([&r] (TTurnInterface& turn)
		{
			if (! r.propagationFlag_.test_and_set())
			{
				r.admissionFlag_.clear();
				R::Engine::OnNodePulse(r, turn);
			}
		});
	}

	void Merge(TransactionInput& other)
	{
		isEmpty_ = isEmpty_ && other.IsEmpty();

		auto myAdmissionPrepareFunc = admissionPrepareFunc_;
		auto otherAdmissionPrepareFunc = other.admissionPrepareFunc_;

		admissionPrepareFunc_ = [=] (TTurnInterface& turn)
		{
			myAdmissionPrepareFunc(turn);
			otherAdmissionPrepareFunc(turn);
		};

		auto myAdmissionCommitFunc = admissionCommitFunc_;
		auto otherAdmissionCommitFunc = other.admissionCommitFunc_;

		admissionCommitFunc_ = [=] (TTurnInterface& turn)
		{
			myAdmissionCommitFunc(turn);
			otherAdmissionCommitFunc(turn);
		};

		auto myPropagationFunc = propagationFunc_;
		auto otherPropagationFunc = other.propagationFunc_;

		propagationFunc_ = [=] (TTurnInterface& turn)
		{
			myPropagationFunc(turn);
			otherPropagationFunc(turn);
		};
	}

	void RunAdmission(TTurnInterface& turn)
	{
		for (const auto& f : concAdmissionPrepareBuf_)
			f(turn);

		admissionPrepareFunc_(turn);

		for (const auto& f : concAdmissionCommitBuf_)
			f(turn);

		admissionCommitFunc_(turn);
	}

	void RunPropagation(TTurnInterface& turn)
	{
		for (const auto& f : concPropagationBuf_)
			f(turn);

		propagationFunc_(turn);
	}

private:
	typedef tbb::queuing_mutex		InputMutexT;
	typedef std::function<void(TTurnInterface&)>	InputClosureT;

	static void	nullFunc(TTurnInterface& other)	{}

	InputClosureT	admissionPrepareFunc_;
	InputClosureT	admissionCommitFunc_;
	InputClosureT	propagationFunc_;

	int		flags_;
	bool	isEmpty_;

	InputMutexT addInputMutex_;

	tbb::concurrent_vector<InputClosureT>	concAdmissionPrepareBuf_;
	tbb::concurrent_vector<InputClosureT>	concAdmissionCommitBuf_;
	tbb::concurrent_vector<InputClosureT>	concPropagationBuf_;
};

template <typename TTurnInterface>
class TransactionData
{
public:
	TransactionData() :
		id_{ INT_MIN },
		curInputPtr_{ &input1_},
		nextInputPtr_{ &input2_ }
	{
	}

	int		Id() const		{ return id_; }
	void	SetId(int id)	{ id_ = id; }

	tbb::concurrent_vector<IObserverNode*>	DetachedObservers;

	TransactionInput<TTurnInterface>& Input()		{ return *curInputPtr_; }
	TransactionInput<TTurnInterface>& NextInput()	{ return *nextInputPtr_; }

	bool ContinueTransaction()
	{
		if (nextInputPtr_->IsEmpty())
			return false;

		id_ = INT_MIN;
		DetachedObservers.clear();

		std::swap(curInputPtr_, nextInputPtr_);
		nextInputPtr_->Reset();
		return true;
	}

private:
	int		id_;

	TransactionInput<TTurnInterface>*	curInputPtr_;
	TransactionInput<TTurnInterface>*	nextInputPtr_;

	TransactionInput<TTurnInterface>	input1_;
	TransactionInput<TTurnInterface>	input2_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// CommitFlags
////////////////////////////////////////////////////////////////////////////////////////
enum CommitFlags
{
	default_commit_flags		= 0,
	allow_transaction_merging	= 1 << 0
};

////////////////////////////////////////////////////////////////////////////////////////
/// ReactiveEngineInterface
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename TEngine
>
struct ReactiveEngineInterface
{
	typedef typename TEngine::NodeInterface		NodeInterface;
	typedef typename TEngine::TurnInterface		TurnInterface;

	static TEngine& Engine()
	{
		static TEngine engine;
		return engine;
	}

	static void OnNodeCreate(NodeInterface& node)
	{
		D::Log().template Append<NodeCreateEvent>(GetObjectId(node), node.GetNodeType());
		Engine().OnNodeCreate(node);
	}

	static void OnNodeDestroy(NodeInterface& node)
	{
		D::Log().template Append<NodeDestroyEvent>(GetObjectId(node));
		Engine().OnNodeDestroy(node);
	}

	static void OnNodeAttach(NodeInterface& node, NodeInterface& parent)
	{
		D::Log().template Append<NodeAttachEvent>(GetObjectId(node), GetObjectId(parent));
		Engine().OnNodeAttach(node, parent);
	}

	static void OnNodeDetach(NodeInterface& node, NodeInterface& parent)
	{
		D::Log().template Append<NodeDetachEvent>(GetObjectId(node), GetObjectId(parent));
		Engine().OnNodeDetach(node, parent);
	}

	static void OnTransactionCommit(TransactionData<TurnInterface>& transaction)
	{
		D::Log().template Append<TransactionBeginEvent>(transaction.Id());
		Engine().OnTransactionCommit(transaction);
		D::Log().template Append<TransactionEndEvent>(transaction.Id());
	}

	static void OnInputNodeAdmission(NodeInterface& node, TurnInterface& turn)
	{
		D::Log().template Append<InputNodeAdmissionEvent>(GetObjectId(node), turn.Id());
		Engine().OnInputNodeAdmission(node, turn);
	}

	static void OnNodePulse(NodeInterface& node, TurnInterface& turn)
	{
		D::Log().template Append<NodePulseEvent>(GetObjectId(node), turn.Id());
		Engine().OnNodePulse(node, turn);
	}

	static void OnNodeIdlePulse(NodeInterface& node, TurnInterface& turn)
	{
		D::Log().template Append<NodeIdlePulseEvent>(GetObjectId(node), turn.Id());
		Engine().OnNodeIdlePulse(node, turn);
	}

	static void OnNodeShift(NodeInterface& node, NodeInterface& oldParent, NodeInterface& newParent, TurnInterface& turn)
	{
		D::Log().template Append<NodeInvalidateEvent>(GetObjectId(node), GetObjectId(oldParent), GetObjectId(newParent), turn.Id());
		Engine().OnNodeShift(node, oldParent, newParent, turn);
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// DomainPolicy
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TEngine,
	typename TLog = NullLog
>
struct DomainPolicy
{
	typedef TEngine			Engine;
	typedef TLog			Log;
};

////////////////////////////////////////////////////////////////////////////////////////
/// DomainBase
////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename TPolicy>
class DomainBase
{
public:
	typedef TPolicy	Policy;

	typedef ReactiveEngineInterface<D, typename Policy::Engine>	Engine;

	////////////////////////////////////////////////////////////////////////////////////////
	/// Aliases for reactives of current domain
	////////////////////////////////////////////////////////////////////////////////////////
	template <typename S>
	using Signal = RSignal<D,S>;

	template <typename S>
	using VarSignal = RVarSignal<D,S>;

	template <typename E = EventToken>
	using Events = REvents<D,E>;

	template <typename E = EventToken>
	using EventSource = REventSource<D,E>;

	using Observer = RObserver<D>;

	////////////////////////////////////////////////////////////////////////////////////////
	/// ObserverRegistry
	////////////////////////////////////////////////////////////////////////////////////////
	static ObserverRegistry<D>& Observers()
	{
		static ObserverRegistry<D> registry;
		return registry;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	/// Log
	////////////////////////////////////////////////////////////////////////////////////////
	static typename Policy::Log& Log()
	{
		static Policy::Log log;
		return log;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	/// MakeVar (higher order signal)
	////////////////////////////////////////////////////////////////////////////////////////
	template
	<
		template <typename Domain_, typename Val_> class TOuter,
		typename TInner
	>
	static inline auto MakeVar(const TOuter<D,TInner>& value)
		-> VarSignal<Signal<TInner>>
	{
		return react::MakeVar<D>(value);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	/// MakeVar
	////////////////////////////////////////////////////////////////////////////////////////
	template <typename S>
	static inline auto MakeVar(const S& value)
		-> VarSignal<S>
	{
		return react::MakeVar<D>(value);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	/// MakeVal
	////////////////////////////////////////////////////////////////////////////////////////
	template <typename S>
	static inline auto MakeVal(const S& value)
		-> Signal<S>
	{
		return react::MakeVal<D>(value);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	/// MakeSignal
	////////////////////////////////////////////////////////////////////////////////////////
	template
	<
		typename TFunc,
		typename ... TArgs
	>
	static inline auto MakeSignal(TFunc func, const Signal<TArgs>& ... args)
		-> Signal<decltype(func(args() ...))>
	{
		typedef decltype(func(args() ...)) S;

		return react::MakeSignal<D>(func, args ...);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	/// MakeEventSource
	////////////////////////////////////////////////////////////////////////////////////////
	template <typename E>
	static inline auto MakeEventSource()
		-> EventSource<E>
	{
		return react::MakeEventSource<D,E>();
	}

	static inline auto MakeEventSource()
		-> EventSource<EventToken>
	{
		return react::MakeEventSource<D>();
	}

	////////////////////////////////////////////////////////////////////////////////////////
	/// Aliases for transactions
	////////////////////////////////////////////////////////////////////////////////////////
	typedef TransactionData<typename Engine::TurnInterface>		TransactionData;
	typedef TransactionInput<typename Engine::TurnInterface>	TransactionInput;

	////////////////////////////////////////////////////////////////////////////////////////
	/// Transaction
	////////////////////////////////////////////////////////////////////////////////////////
	class Transaction
	{
	public:
		Transaction() :
			flags_{ defaultCommitFlags_ }
		{
		}

		Transaction(int flags) :
			flags_{ flags }
		{
		}

		void Commit()
		{
			REACT_ASSERT(committed_ == false, "Transaction already committed.");

			if (!committed_)
			{
				committed_ = true;

				do
				{
					data_.SetId(nextTransactionId());
					data_.Input().SetFlags(flags_);
					Engine::OnTransactionCommit(data_);

					// Apply detachments requested by DetachThisObserver
					for (auto* o : data_.DetachedObservers)
						Observers().Unregister(o);
				}
				while (data_.ContinueTransaction());
			}
		}

		TransactionData& Data()
		{
			return data_;
		}

	private:

		int		flags_;
		bool	committed_ = false;

		TransactionData		data_;
	};

	////////////////////////////////////////////////////////////////////////////////////////
	/// ScopedTransaction
	////////////////////////////////////////////////////////////////////////////////////////
	class ScopedTransaction
	{
	public:
		ScopedTransaction() :
			transaction_{ defaultCommitFlags_ }
		{
			REACT_ASSERT(ScopedTransactionInput::IsNull(), "Nested scoped transactions are not supported.");
			REACT_ASSERT(TransactionInputContinuation::IsNull(), "Scoped transactions are not supported inside of observer functions.");
			ScopedTransactionInput::Set(&transaction_.Data().Input());
		}

		ScopedTransaction(int flags) :
			transaction_{ flags }
		{
			REACT_ASSERT(ScopedTransactionInput::IsNull(), "Nested scoped transactions are not supported.");
			REACT_ASSERT(TransactionInputContinuation::IsNull(), "Scoped transactions are not supported inside of observer functions.");
			ScopedTransactionInput::Set(&transaction_.Data().Input());
		}

		~ScopedTransaction()
		{
			ScopedTransactionInput::Reset();
			transaction_.Commit();
		}

		TransactionData& Data()
		{
			return transaction_.Data();
		}

	private:
		Transaction		transaction_;
	};

	////////////////////////////////////////////////////////////////////////////////////////
	/// Scoped transaction input
	////////////////////////////////////////////////////////////////////////////////////////
	struct ScopedTransactionInput : public ThreadLocalStaticPtr<TransactionInput> {};

	////////////////////////////////////////////////////////////////////////////////////////
	/// Transaction input continuation
	////////////////////////////////////////////////////////////////////////////////////////
	struct TransactionInputContinuation : public ThreadLocalStaticPtr<TransactionInput> {};

	////////////////////////////////////////////////////////////////////////////////////////
	/// Default commit flags
	////////////////////////////////////////////////////////////////////////////////////////
	static void SetDefaultCommitFlags(int flags)
	{
		defaultCommitFlags_ = flags;
	}

	static int GetDefaultCommitFlags()
	{
		return defaultCommitFlags_;
	}

private:
	static __declspec(thread) int defaultCommitFlags_;

	static std::atomic<int> nextTransactionId_;

	static int nextTransactionId()
	{
		int curId = nextTransactionId_.fetch_add(1, std::memory_order_relaxed);

		if (curId == INT_MAX)
			nextTransactionId_.fetch_sub(INT_MAX);

		return curId;
	}

	// Can't be instantiated
	DomainBase();
};

template <typename D, typename TPolicy>
std::atomic<int> DomainBase<D,TPolicy>::nextTransactionId_(0);

template <typename D, typename TPolicy>
int DomainBase<D,TPolicy>::defaultCommitFlags_(0);

////////////////////////////////////////////////////////////////////////////////////////
/// Ensure singletons are created immediately after domain declaration (TODO hax)
////////////////////////////////////////////////////////////////////////////////////////
namespace impl
{

template <typename D>
class DomainInitializer
{
public:
	DomainInitializer()
	{
		D::Log();
		typename D::Engine::Engine();
	}
};

} // ~namespace react::impl

#define REACTIVE_DOMAIN(name, ...) \
	struct name : public react::DomainBase<name, react::DomainPolicy<__VA_ARGS__ >> {}; \
	react::impl::DomainInitializer< name > name ## _initializer_;

} // ~namespace react