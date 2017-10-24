
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "gtest/gtest.h"

#include "react/common/syncpoint.h"

#include <chrono>
#include <cstdio>
#include <thread>

using namespace react;

///////////////////////////////////////////////////////////////////////////////////////////////////
TEST(SyncPointTest, DependencyCreation)
{
    SyncPoint sp;

    {
        SyncPoint::Dependency dep1(sp);
        SyncPoint::Dependency dep2(sp);
        SyncPoint::Dependency dep3(sp);

        std::vector<SyncPoint::Dependency> deps1 = { dep1, dep2, dep3 };
        std::vector<SyncPoint::Dependency> deps2 = { dep1 };

        SyncPoint::Dependency dep4( begin(deps1), end(deps1) );
        

        SyncPoint::Dependency dep5;

        dep5 = std::move(dep4);

        SyncPoint::Dependency dep6;
        dep6 = dep5;
        SyncPoint::Dependency dep7( dep6 );

        SyncPoint::Dependency dep8( begin(deps2), end(deps2) );
    }

    bool done = sp.WaitFor(std::chrono::milliseconds(1));
    EXPECT_EQ(true, done);
}

TEST(SyncPointTest, SingleWait)
{
    SyncPoint sp;

    SyncPoint::Dependency dep(sp);

    int output = 0;

    std::thread t1([storedDep = std::move(dep), &output] ()
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            output = 1;
        });

    sp.Wait();
    
    EXPECT_EQ(1, output);

    t1.join();
}

TEST(SyncPointTest, MultiWait)
{
    SyncPoint sp;

    SyncPoint::Dependency dep1(sp);
    SyncPoint::Dependency dep2(sp);
    SyncPoint::Dependency dep3(sp);

    int output1 = 0;
    int output2 = 0;
    int output3 = 0;

    std::thread t1([storedDep = std::move(dep1), &output1] ()
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            output1 = 1;
        });

    std::thread t2([storedDep = std::move(dep2), &output2] ()
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            output2 = 2;
        });

    std::thread t3([storedDep = std::move(dep3), &output3] ()
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            output3 = 3;
        });

    
    sp.Wait();
    
    EXPECT_EQ(1, output1);
    EXPECT_EQ(2, output2);
    EXPECT_EQ(3, output3);

    t1.join();
    t2.join();
    t3.join();
}

TEST(SyncPointTest, MultiWaitFor)
{
    SyncPoint sp;

    SyncPoint::Dependency dep1(sp);
    SyncPoint::Dependency dep2(sp);
    SyncPoint::Dependency dep3(sp);

    int output1 = 0;
    int output2 = 0;
    int output3 = 0;

    std::thread t1([storedDep = std::move(dep1), &output1] ()
        {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            output1 = 1;
        });

    std::thread t2([storedDep = std::move(dep2), &output2] ()
        {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            output2 = 2;
        });

    std::thread t3([storedDep = std::move(dep3), &output3] ()
        {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            output3 = 3;
        });

    
    bool done = sp.WaitFor(std::chrono::milliseconds(1));
    
    EXPECT_FALSE(done);

    EXPECT_EQ(0, output1);
    EXPECT_EQ(0, output2);
    EXPECT_EQ(0, output3);

    done = sp.WaitFor(std::chrono::seconds(10));

    EXPECT_TRUE(done);

    EXPECT_EQ(1, output1);
    EXPECT_EQ(2, output2);
    EXPECT_EQ(3, output3);

    t1.join();
    t2.join();
    t3.join();
}