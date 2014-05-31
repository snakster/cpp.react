
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gtest/gtest.h"

#include <algorithm>
#include <thread>
#include <vector>

#include "react/Domain.h"
#include "react/Signal.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalTest fixture
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TEngine>
class TransactionTest : public testing::Test
{
public:
    REACTIVE_DOMAIN(MyDomain, TEngine);
};

TYPED_TEST_CASE_P(TransactionTest);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Concurrent transactions test 1
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(TransactionTest, Concurrent1)
{
    using D = typename Concurrent1::MyDomain;

    std::vector<int> results;

    auto n1 = MakeVar<D>(1);
    auto n2 = n1 + 1;
    auto n3 = n2 + n1 + 1;
    auto n4 = n3 + 1;
    auto n5 = n4 + n3 + n1 + 1;
    auto n6 = n5 + 1;
    auto n7 = n6 + n5 + 1;
    auto n8 = n7 + 1;
    auto n9 = n8 + n7 + n5 + n1 + 1;
    auto n10 = n9 + 1;
    auto n11 = n10 + n9 + 1;
    auto n12 = n11 + 1;
    auto n13 = n12 + n11 + n9 + 1;
    auto n14 = n13 + 1;
    auto n15 = n14 + n13 + 1;
    auto n16 = n15 + 1;
    auto n17 = n16 + n15 + n13 + n9 + 1;

    Observe(n17, [&] (int v)
    {
        results.push_back(v);
    });

    n1 <<= 10;        // 7732
    n1 <<= 100;        // 68572
    n1 <<= 1000;    // 676972

    ASSERT_EQ(results.size(), 3);

    ASSERT_TRUE(std::find(results.begin(), results.end(), 7732) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), 68572) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), 676972) != results.end());

    // Reset
    n1 <<= 1;
    results.clear();

    // Now do the same from 3 threads

    std::thread t1([&]    { n1 <<= 10; });
    std::thread t2([&]    { n1 <<= 100; });
    std::thread t3([&]    { n1 <<= 1000; });

    t1.join();
    t2.join();
    t3.join();

    ASSERT_EQ(results.size(), 3);

    ASSERT_TRUE(std::find(results.begin(), results.end(), 7732) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), 68572) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), 676972) != results.end());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Concurrent transactions test 2
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(TransactionTest, Concurrent2)
{
    using D = typename Concurrent2::MyDomain;

    std::vector<int> results;

    auto in = MakeVar<D>(-1);

    // 1. Generate graph
    Signal<D,int> n0 = in;

    auto next = n0;

    for (int i=0; i < 100; i++)
    {
        auto q = next + 0;
        next = q;
    }

    Observe(next, [&] (int v)
    {
        results.push_back(v);
    });

    // 2. Send events
    std::thread t1([&]
    {
        for (int i=0; i<100; i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 50));
            in <<= i;
        }
    });

    std::thread t2([&]
    {
        for (int i=100; i<200; i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 50));
            in <<= i;
        }
    });

    std::thread t3([&]
    {
        for (int i=200; i<300; i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 50));
            in <<= i;
        }
    });

    t1.join();
    t2.join();
    t3.join();

    ASSERT_EQ(results.size(), 300);

    for (int i=0; i<300; i++)
    {
        ASSERT_TRUE(std::find(results.begin(), results.end(), i) != results.end());
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Concurrent transactions test 3
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(TransactionTest, Concurrent3)
{
    using D = typename Concurrent3::MyDomain;

    std::vector<int> results;

    std::function<int(int)> f_0 = [] (int a) -> int
    {
        for (int i = 0; i<(a+1)*100; i++);
        return a + 1;
    };

    std::function<int(int,int)> f_n = [] (int a, int b) -> int
    {
        for (int i = 0; i<(a+b)*100; i++);
        return a + b;
    };

    auto n1 = MakeVar<D>(1);
    auto n2 = n1 ->* f_0;
    auto n3 = ((n2, n1) ->* f_n) ->* f_0;
    auto n4 = n3 ->* f_0;
    auto n5 = ((((n4, n3) ->* f_n), n1) ->* f_n) ->* f_0;
    auto n6 = n5 ->* f_0;
    auto n7 = ((n6, n5) ->* f_n) ->* f_0;
    auto n8 = n7 ->* f_0;
    auto n9 = ((((((n8, n7) ->* f_n), n5) ->* f_n), n1) ->* f_n) ->* f_0;
    auto n10 = n9 ->* f_0;
    auto n11 = ((n10, n9) ->* f_n) ->* f_0;
    auto n12 = n11 ->* f_0;
    auto n13 = ((((n12, n11) ->* f_n), n9) ->* f_n) ->* f_0;
    auto n14 = n13 ->* f_0;
    auto n15 = ((n14, n13) ->* f_n) ->* f_0;
    auto n16 = n15 ->* f_0;
    auto n17 = ((((((n16, n15) ->* f_n), n13) ->* f_n), n9) ->* f_n)  ->* f_0;

    Observe(n17, [&] (int v)
    {
        results.push_back(v);
    });

    n1 <<= 1000;    // 676972
    n1 <<= 100;     // 68572
    n1 <<= 10;      // 7732


    ASSERT_EQ(results.size(), 3);

    ASSERT_TRUE(std::find(results.begin(), results.end(), 7732) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), 68572) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), 676972) != results.end());

    // Reset
    n1 <<= 1;
    results.clear();
    
    std::thread t3([&]    { n1 <<= 1000; });
    std::thread t2([&]    { n1 <<= 100; });
    std::thread t1([&]    { n1 <<= 10; });
    
    t3.join();
    t2.join();
    t1.join();


    ASSERT_EQ(results.size(), 3);

    ASSERT_TRUE(std::find(results.begin(), results.end(), 7732) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), 68572) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), 676972) != results.end());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Merging1
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(TransactionTest, Merging1)
{
    using D = typename Merging1::MyDomain;

    std::vector<int> results;

    std::atomic<bool> shouldSpin(false);

    std::function<int(int)> f = [&shouldSpin] (int a) -> int
    {
        while (shouldSpin);

        return a;
    };

    auto n1 = MakeVar<D>(1);
    auto n2 = n1 ->* f;

    Observe(n2, [&] (int v)
    {
        results.push_back(v);
    });

    // Todo: improve this as it'll fail occasionally
    shouldSpin = true;
    std::thread t1([&] {
        D::DoTransaction(enable_input_merging, [&] {
            n1 <<= 2;
        });
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    std::thread t2([&] {
        D::DoTransaction(enable_input_merging, [&] {
            n1 <<= 3;
        });
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::thread t3([&] {
        D::DoTransaction(enable_input_merging, [&] {
            n1 <<= 4;
        });
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::thread t4([&] {
        D::DoTransaction(enable_input_merging, [&] {
            n1 <<= 5;
        });
        
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    shouldSpin = false;

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    ASSERT_EQ(results.size(), 2);
    ASSERT_TRUE(results[0] == 2);
    ASSERT_TRUE(results[1] == 5);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_TYPED_TEST_CASE_P
(
    TransactionTest,
    Concurrent1,
    Concurrent2,
    Concurrent3,
    Merging1
);

} // ~namespace
