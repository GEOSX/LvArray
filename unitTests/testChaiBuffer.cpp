/*
 * Copyright (c) 2020, Lawrence Livermore National Security, LLC and LvArray contributors.
 * All rights reserved.
 * See the LICENSE file for details.
 * SPDX-License-Identifier: (BSD-3-Clause)
 */

/**
 * @file testBuffers.cpp
 */

// Source includes
#include "testUtils.hpp"
#include "ChaiBuffer.hpp"

// TPL includes
#include <gtest/gtest.h>

// System includes
#include <vector>
#include <random>


namespace LvArray
{
namespace testing
{

template< typename T >
class ChaiBufferTest : public ::testing::Test
{
public:

  void testAllocatorConstruction()
  {
    umpire::ResourceManager & rm = umpire::ResourceManager::getInstance();
    auto hostPool = rm.makeAllocator< umpire::strategy::DynamicPool >( "HOST_pool", rm.getAllocator( "HOST" ) );

  #if defined( LVARRAY_USE_CUDA )
    auto devicePool = rm.makeAllocator< umpire::strategy::DynamicPool >( "DEVICE_pool", rm.getAllocator( "DEVICE" ) );
    std::initializer_list< MemorySpace > const spaces = { MemorySpace::CPU, MemorySpace::GPU };
    std::initializer_list< umpire::Allocator > const allocators = { hostPool, devicePool };
  #else
    std::initializer_list< MemorySpace > const spaces = { MemorySpace::CPU };
    std::initializer_list< umpire::Allocator > const allocators = { hostPool };
  #endif

    ChaiBuffer< T > buffer( spaces, allocators );

    int const size = 100;
    buffer.reallocate( 0, MemorySpace::CPU, size );

    for( int i = 0; i < size; ++i )
    {
      new ( &buffer[ i ] ) T( i );
    }

    EXPECT_EQ( rm.getAllocator( buffer.data() ).getName(), "HOST_pool" );

  #if defined( LVARRAY_USE_CUDA )
    buffer.move( MemorySpace::GPU, true );
    EXPECT_EQ( rm.getAllocator( buffer.data() ).getName(), "DEVICE_pool" );

    buffer.move( MemorySpace::CPU, true );
    EXPECT_EQ( rm.getAllocator( buffer.data() ).getName(), "HOST_pool" );
  #endif

    bufferManipulation::free( buffer, size );
  }


#if defined( LVARRAY_USE_CUDA )
  void testMove()
  {
    ChaiBuffer< T > buffer( true );

    int const size = 100;
    buffer.reallocate( 0, MemorySpace::CPU, size );
    EXPECT_EQ( buffer.getPreviousSpace(), MemorySpace::CPU );

    for( int i = 0; i < size; ++i )
    {
      new ( &buffer[ i ] ) T( i );
    }

    buffer.move( MemorySpace::GPU, true );
    EXPECT_EQ( buffer.getPreviousSpace(), MemorySpace::GPU );
    T * const devPtr = buffer.data();

    forall< parallelDevicePolicy< 32 > >( size, [devPtr] LVARRAY_DEVICE ( int const i )
    {
      devPtr[ i ] += devPtr[ i ];
    } );

    // Check that the device changes are seen on the host. Then modify the values without touching.
    buffer.move( MemorySpace::CPU, false );
    EXPECT_EQ( buffer.getPreviousSpace(), MemorySpace::CPU );
    for( int i = 0; i < size; ++i )
    {
      EXPECT_EQ( buffer[ i ], T( i ) + T( i ) );
      buffer[ i ] = T( 0 );
    }

    buffer.move( MemorySpace::GPU, true );
    EXPECT_EQ( buffer.getPreviousSpace(), MemorySpace::GPU );
    forall< parallelDevicePolicy< 32 > >( size, [devPtr] LVARRAY_DEVICE ( int const i )
    {
      devPtr[ i ] += devPtr[ i ];
    } );

    buffer.move( MemorySpace::CPU, false );
    EXPECT_EQ( buffer.getPreviousSpace(), MemorySpace::CPU );
    for( int i = 0; i < size; ++i )
    {
      EXPECT_EQ( buffer[ i ], T( i ) + T( i ) + T( i ) + T( i ) );
    }

    bufferManipulation::free( buffer, size );
  }

  void testCapture()
  {
    ChaiBuffer< T > buffer( true );

    int const size = 100;
    buffer.reallocate( 0, MemorySpace::CPU, size );
    EXPECT_EQ( buffer.getPreviousSpace(), MemorySpace::CPU );

    for( int i = 0; i < size; ++i )
    {
      new ( &buffer[ i ] ) T( i );
    }

    forall< parallelDevicePolicy< 32 > >( size, [buffer] LVARRAY_DEVICE ( int const i )
    {
      buffer[ i ] += buffer[ i ];
    } );

    EXPECT_EQ( buffer.getPreviousSpace(), MemorySpace::GPU );


    // Check that the device changes are seen on the host. Then modify the values without touching.
    ChaiBuffer< T const > constBuffer( buffer );
    forall< serialPolicy >( size, [constBuffer] ( int const i )
    {
      EXPECT_EQ( constBuffer[ i ], T( i ) + T( i ) );
      const_cast< T & >( constBuffer[ i ] ) = T( 0 );
    } );

    EXPECT_EQ( buffer.getPreviousSpace(), MemorySpace::CPU );
    EXPECT_EQ( constBuffer.getPreviousSpace(), MemorySpace::CPU );

    forall< parallelDevicePolicy< 32 > >( size, [buffer] LVARRAY_DEVICE ( int const i )
    {
      buffer[ i ] += buffer[ i ];
    } );

    EXPECT_EQ( buffer.getPreviousSpace(), MemorySpace::GPU );

    forall< serialPolicy >( size, [constBuffer] ( int const i )
    {
      EXPECT_EQ( constBuffer[ i ], T( i ) + T( i ) + T( i ) + T( i ) );
    } );

    EXPECT_EQ( buffer.getPreviousSpace(), MemorySpace::CPU );
    EXPECT_EQ( constBuffer.getPreviousSpace(), MemorySpace::CPU );

    bufferManipulation::free( buffer, size );
  }

  void testDeviceRealloc()
  {
    ChaiBuffer< T > buffer( true );

    int const size = 100;
    buffer.reallocate( 0, MemorySpace::GPU, size );

    T * const devPtr = buffer.data();
    forall< parallelDevicePolicy< 32 > >( size, [devPtr] LVARRAY_DEVICE ( int const i )
    {
      new ( &devPtr[ i ] ) T( i );
    } );

    buffer.move( MemorySpace::CPU, true );
    for( int i = 0; i < size; ++i )
    {
      EXPECT_EQ( buffer[ i ], T( i ) );
    }

    bufferManipulation::free( buffer, size );
  }
#endif
};

/// The list of types to instantiate ChaiBufferTest with.
using ChaiBufferTestTypes = ::testing::Types<
  int
  >;

TYPED_TEST_SUITE( ChaiBufferTest, ChaiBufferTestTypes, );

TYPED_TEST( ChaiBufferTest, AllocatorConstruction )
{
  this->testAllocatorConstruction();
}

#if defined( LVARRAY_USE_CUDA )

TYPED_TEST( ChaiBufferTest, Move )
{
  this->testMove();
}

TYPED_TEST( ChaiBufferTest, Capture )
{
  this->testCapture();
}

TYPED_TEST( ChaiBufferTest, DeviceRealloc )
{
  this->testDeviceRealloc();
}

#endif

} // namespace testing
} // namespace LvArray

// This is the default gtest main method. It is included for ease of debugging.
int main( int argc, char * * argv )
{
  ::testing::InitGoogleTest( &argc, argv );
  int const result = RUN_ALL_TESTS();
  return result;
}
