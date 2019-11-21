/*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Copyright (c) 2019, Lawrence Livermore National Security, LLC.
 *
 * Produced at the Lawrence Livermore National Laboratory
 *
 * LLNL-CODE-746361
 *
 * All rights reserved. See COPYRIGHT for details.
 *
 * This file is part of the GEOSX Simulation Framework.
 *
 * GEOSX is a free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License (as published by the
 * Free Software Foundation) version 2.1 dated February 1999.
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

/**
 * @file ArrayOfArrays.hpp
 */

#pragma once

#include "ArrayOfArraysView.hpp"

namespace LvArray
{

// Forward declaration of the ArrayOfSets class so that we can define the stealFrom method.
template< class T, class INDEX_TYPE >
class ArrayOfSets;

/**
 * @class ArrayOfArrays
 * @brief This class implements an array of arrays like object with contiguous storage.
 * @tparam T the type stored in the arrays.
 * @tparam INDEX_TYPE the integer to use for indexing.
 */
template< class T, typename INDEX_TYPE=std::ptrdiff_t >
class ArrayOfArrays : protected ArrayOfArraysView< T, INDEX_TYPE >
{
public:

  // Aliasing public methods of ArrayOfArraysView.
  using ArrayOfArraysView< T, INDEX_TYPE >::toViewC;
  using ArrayOfArraysView< T, INDEX_TYPE >::toViewCC;
  using ArrayOfArraysView< T, INDEX_TYPE >::sizeOfArray;
  using ArrayOfArraysView< T, INDEX_TYPE >::capacity;
  using ArrayOfArraysView< T, INDEX_TYPE >::capacityOfArray;
  using ArrayOfArraysView< T, INDEX_TYPE >::operator[];
  using ArrayOfArraysView< T, INDEX_TYPE >::operator();
  using ArrayOfArraysView< T, INDEX_TYPE >::getIterableArray;
  using ArrayOfArraysView< T, INDEX_TYPE >::atomicAppendToArray;
  using ArrayOfArraysView< T, INDEX_TYPE >::eraseFromArray;

  /**
   * @brief Return the number of arrays.
   * @note This needs is duplicated here for the intel compiler on cori.
   */
  inline
  INDEX_TYPE size() const restrict_this
  { return m_numArrays; }

  /**
   * @brief Constructor.
   * @param [in] numArrays the number of arrays.
   * @param [in] defaultArrayCapacity the initial capacity of each array.
   */
  inline
  ArrayOfArrays( INDEX_TYPE const numArrays=0, INDEX_TYPE const defaultArrayCapacity=0 ) restrict_this:
    ArrayOfArraysView< T, INDEX_TYPE >()
  {
    resize( numArrays, defaultArrayCapacity );
    setName( "" );
  }

  /**
   * @brief Copy constructor, performs a deep copy.
   * @param [in] src the ArrayOfArrays to copy.
   */
  inline
  ArrayOfArrays( ArrayOfArrays const & src ):
    ArrayOfArraysView< T, INDEX_TYPE >()
  { *this = src; }

  /**
   * @brief Default move constructor, performs a shallow copy.
   * @param [in/out] src the ArrayOfArrays to be moved from.
   */
  inline
  ArrayOfArrays( ArrayOfArrays && ) = default;

  /**
   * @brief Destructor, frees the values, sizes and offsets buffers.
   */
  ~ArrayOfArrays()
  { ArrayOfArraysView< T, INDEX_TYPE >::free(); }

  /**
   * @brief Conversion operator to ArrayOfArraysView<T, INDEX_TYPE const>.
   */
  CONSTEXPRFUNC inline
  operator ArrayOfArraysView< T, INDEX_TYPE const > const &
  () const restrict_this
  { return reinterpret_cast< ArrayOfArraysView< T, INDEX_TYPE const > const & >(*this); }

  /**
   * @brief Method to convert to ArrayOfArraysView<T, INDEX_TYPE const>. Use this method when
   *        the above UDC isn't invoked, this usually occurs with template argument deduction.
   */
  CONSTEXPRFUNC inline
  ArrayOfArraysView< T, INDEX_TYPE const > const & toView() const restrict_this
  { return *this; }

  /**
   * @brief Conversion operator to ArrayOfArraysView<T, INDEX_TYPE const, true>.
   *        Although ArrayOfArraysView defines this operator nvcc won't let us alias it so
   *        it is redefined here.
   */
  CONSTEXPRFUNC inline
  operator ArrayOfArraysView< T, INDEX_TYPE const, true > const &
  () const restrict_this
  { return toViewC(); }

  /**
   * @brief Conversion operator to ArrayOfArraysView<T const, INDEX_TYPE const, true>.
   *        Although ArrayOfArraysView defines this operator nvcc won't let us alias it so
   *        it is redefined here.
   */
  CONSTEXPRFUNC inline
  operator ArrayOfArraysView< T const, INDEX_TYPE const, true > const &
  () const restrict_this
  { return toViewCC(); }

  /**
   * @brief Copy assignment operator, performs a deep copy.
   * @param [in] src the ArrayOfArrays to copy.
   */
  inline
  ArrayOfArrays & operator=( ArrayOfArrays const & src ) restrict_this
  {
    ArrayOfArraysView< T, INDEX_TYPE >::setEqualTo( src.m_numArrays,
                                                    src.m_offsets[ src.m_numArrays ],
                                                    src.m_offsets,
                                                    src.m_sizes,
                                                    src.m_values );
    return *this;
  }

  /**
   * @brief Default move assignment operator, performs a shallow copy.
   * @param [in] src the ArrayOfArrays to be moved from.
   */
  inline
  ArrayOfArrays & operator=( ArrayOfArrays && src ) = default;

  /**
   * @brief Steal the resources from an ArrayOfSets and convert it to an ArrayOfArrays.
   * @param [in/out] src the ArrayOfSets to convert.
   */
  inline
  void stealFrom( ArrayOfSets< T, INDEX_TYPE > && src )
  {
    // Reinterpret cast to ArrayOfArraysView so that we don't have to include ArrayOfSets.hpp.
    ArrayOfArraysView< T, INDEX_TYPE > && srcView = reinterpret_cast< ArrayOfArraysView< T, INDEX_TYPE > && >( src );

    m_numArrays = srcView.m_numArrays;
    srcView.m_numArrays = 0;

    m_offsets = std::move( srcView.m_offsets );
    m_sizes = std::move( srcView.m_sizes );
    m_values = std::move( srcView.m_values );
  }

  /**
   * @brief Move to the given memory space, optionally touching it.
   * @param [in] space the memory space to move to.
   * @param [in] touch whether to touch the memory in the space or not.
   */
  void move( chai::ExecutionSpace const space, bool const touch=true ) restrict_this
  { ArrayOfArraysView< T, INDEX_TYPE >::move( space, touch ); }

  /**
   * @brief Touch in the given memory space.
   * @param [in] space the memory space to touch.
   */
  void registerTouch( chai::ExecutionSpace const space ) restrict_this
  { ArrayOfArraysView< T, INDEX_TYPE >::registerTouch( space ); }

  /**
   * @brief Reserve space for the given number of arrays.
   * @param [in] newCapacity the new minimum capacity for the number of arrays.
   */
  void reserve( INDEX_TYPE const newCapacity )
  { ArrayOfArraysView< T, INDEX_TYPE >::reserve( newCapacity ); }

  /**
   * @brief Reserve space for the given number of values.
   * @param [in] newValueCapacity the new minimum capacity for the number of values across all arrays.
   */
  void reserveValues( INDEX_TYPE const newValueCapacity )
  { ArrayOfArraysView< T, INDEX_TYPE >::reserveValues( newValueCapacity ); }

  /**
   * @brief Set the number of arrays.
   * @param [in] newSize the new number of arrays.
   * @note We need this method in addition to the following resize method because of SFINAE requirements.
   */
  void resize( INDEX_TYPE const numArrays )
  { ArrayOfArraysView< T, INDEX_TYPE >::resize( numArrays, 0 ); }

  /**
   * @brief Set the number of arrays.
   * @param [in] newSize the new number of arrays.
   * @param [in] defaultArrayCapacity the default capacity for each new array.
   */
  void resize( INDEX_TYPE const numArrays, INDEX_TYPE const defaultArrayCapacity )
  { ArrayOfArraysView< T, INDEX_TYPE >::resize( numArrays, defaultArrayCapacity ); }

  /**
   * @brief Append an array.
   * @param [in] n the size of the array.
   */
  void appendArray( INDEX_TYPE const n ) restrict_this
  {
    LVARRAY_ASSERT( arrayManipulation::isPositive( n ) );

    INDEX_TYPE const maxOffset = m_offsets[ m_numArrays ];
    bufferManipulation::pushBack( m_offsets, m_numArrays + 1, maxOffset );
    bufferManipulation::pushBack( m_sizes, m_numArrays, 0 );
    ++m_numArrays;

    resizeArray( m_numArrays - 1, n );
  }

  /**
   * @brief Append an array.
   * @param [in] values the values of the array to append.
   * @param [in] n the number of values.
   */
  void appendArray( T const * const values, INDEX_TYPE const n ) restrict_this
  {
    INDEX_TYPE const maxOffset = m_offsets[ m_numArrays ];
    bufferManipulation::pushBack( m_offsets, m_numArrays + 1, maxOffset );
    bufferManipulation::pushBack( m_sizes, m_numArrays, 0 );
    ++m_numArrays;

    appendToArray( m_numArrays - 1, values, n );
  }

  /**
   * @brief Insert an array.
   * @param [in] i the position to insert the array.
   * @param [in] values the values of the array to insert.
   * @param [in] n the number of values.
   */
  void insertArray( INDEX_TYPE const i, T const * const values, INDEX_TYPE const n )
  {
    ARRAYOFARRAYS_CHECK_INSERT_BOUNDS( i );
    LVARRAY_ASSERT( arrayManipulation::isPositive( n ) );
    LVARRAY_ASSERT( n == 0 || values != nullptr );

    // Insert an array of capacity zero at the given location
    INDEX_TYPE const offset = m_offsets[i];
    bufferManipulation::insert( m_offsets, m_numArrays + 1, i + 1, offset );
    bufferManipulation::insert( m_sizes, m_numArrays, i, 0 );
    ++m_numArrays;

    // Append to the new array
    appendToArray( i, values, n );
  }

  /**
   * @brief Erase an array.
   * @param [in] i the position of the array to erase.
   */
  void eraseArray( INDEX_TYPE const i )
  {
    ARRAYOFARRAYS_CHECK_BOUNDS( i );

    setCapacityOfArray( i, 0 );
    bufferManipulation::erase( m_offsets, m_numArrays + 1, i + 1 );
    bufferManipulation::erase( m_sizes, m_numArrays, i );
    --m_numArrays;
  }

  /**
   * @brief Compress the arrays so that the values of each array are contiguous with no extra capacity in between.
   * @note This method doesn't free any memory.
   */
  void compress()
  { ArrayOfArraysView< T, INDEX_TYPE >::compress(); }

  /**
   * @brief Append a value to an array.
   * @param [in] i the array to append to.
   * @param [in] value the value to append.
   */
  void appendToArray( INDEX_TYPE const i, T const & value ) restrict_this
  {
    dynamicallyGrowArray( i, 1 );
    ArrayOfArraysView< T, INDEX_TYPE >::appendToArray( i, value );
  }

  /**
   * @brief Append a value to an array.
   * @param [in] i the array to append to.
   * @param [in/out] value the value to append.
   */
  void appendToArray( INDEX_TYPE const i, T && value ) restrict_this
  {
    dynamicallyGrowArray( i, 1 );
    ArrayOfArraysView< T, INDEX_TYPE >::appendToArray( i, std::move( value ) );
  }

  /**
   * @brief Append values to an array.
   * @param [in] i the array to append to.
   * @param [in] values the values to append.
   * @param [in] n the number of values to append.
   */
  void appendToArray( INDEX_TYPE const i, T const * const values, INDEX_TYPE const n ) restrict_this
  {
    dynamicallyGrowArray( i, n );
    ArrayOfArraysView< T, INDEX_TYPE >::appendToArray( i, values, n );
  }

  /**
   * @brief Insert a value into an array.
   * @param [in] i the array to insert into.
   * @param [in] j the position at which to insert.
   * @param [in] value the value to insert.
   */
  void insertIntoArray( INDEX_TYPE const i, INDEX_TYPE const j, T const & value )
  {
    dynamicallyGrowArray( i, 1 );
    ArrayOfArraysView< T, INDEX_TYPE >::insertIntoArray( i, j, value );
  }

  /**
   * @brief Insert a value into an array.
   * @param [in] i the array to insert into.
   * @param [in] j the position at which to insert.
   * @param [in/out] value the value to insert.
   */
  void insertIntoArray( INDEX_TYPE const i, INDEX_TYPE const j, T && value )
  {
    dynamicallyGrowArray( i, 1 );
    ArrayOfArraysView< T, INDEX_TYPE >::insertIntoArray( i, j, std::move( value ) );
  }

  /**
   * @brief Insert values into an array.
   * @param [in] i the array to insert into.
   * @param [in] j the position at which to insert.
   * @param [in] values the values to insert.
   * @param [in] n the number of values to insert.
   */
  void insertIntoArray( INDEX_TYPE const i, INDEX_TYPE const j, T const * const values, INDEX_TYPE const n )
  {
    dynamicallyGrowArray( i, n );
    ArrayOfArraysView< T, INDEX_TYPE >::insertIntoArray( i, j, values, n );
  }

  /**
   * @brief Set the number of values in an array.
   * @tparam ARGS variadic template parameter of the types used to initialize any new values with.
   * @param [in] i the array to resize.
   * @param [in] newSize the value to set the size of the array to.
   * @param [in] args variadic parameter pack of the arguments used to initialize any new values with.
   */
  template< class ... ARGS >
  void resizeArray( INDEX_TYPE const i, INDEX_TYPE const newSize, ARGS && ... args )
  {
    ARRAYOFARRAYS_CHECK_BOUNDS( i );
    LVARRAY_ASSERT( arrayManipulation::isPositive( newSize ) );

    if( newSize > capacityOfArray( i ) )
    {
      setCapacityOfArray( i, newSize );
    }

    INDEX_TYPE const prevSize = sizeOfArray( i );
    T * const values = (*this)[i];
    arrayManipulation::resize( values, prevSize, newSize, std::forward< ARGS >( args )... );
    m_sizes[ i ] = newSize;
  }

  /**
   * @brief Clear the given array.
   * @param[in] i the index of the array to clear.
   */
  void clearArray( INDEX_TYPE const i )
  { resizeArray( i, 0 ); }

  /**
   * @brief Set the capacity of an array.
   * @param [in] i the array to set the capacity of.
   * @param [in] newCapacity the value to set the capacity of the array to.
   */
  void setCapacityOfArray( INDEX_TYPE const i, INDEX_TYPE const newCapacity )
  { ArrayOfArraysView< T, INDEX_TYPE >::setCapacityOfArray( i, newCapacity ); }

  void setName( std::string const & name )
  {
    ArrayOfArraysView< T, INDEX_TYPE >::template setName< decltype( *this ) >( name );
  }

private:

  /**
   * @brief Dynamically grow the capacity of an array.
   * @param [in] i the array to grow.
   * @param [in] increase the increase in the size of the array.
   */
  void dynamicallyGrowArray( INDEX_TYPE const i, INDEX_TYPE const increase )
  {
    LVARRAY_ASSERT( arrayManipulation::isPositive( increase ) );

    INDEX_TYPE const newArraySize = sizeOfArray( i ) + increase;
    if( newArraySize > capacityOfArray( i ))
    {
      setCapacityOfArray( i, 2 * newArraySize );
    }
  }

  using ArrayOfArraysView< T, INDEX_TYPE >::m_numArrays;
  using ArrayOfArraysView< T, INDEX_TYPE >::m_offsets;
  using ArrayOfArraysView< T, INDEX_TYPE >::m_sizes;
  using ArrayOfArraysView< T, INDEX_TYPE >::m_values;
};

} /* namespace LvArray */