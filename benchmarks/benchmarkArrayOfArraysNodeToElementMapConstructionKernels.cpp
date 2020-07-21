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

// Source includes
#include "benchmarkArrayOfArraysNodeToElementMapConstructionKernels.hpp"

namespace LvArray
{
namespace benchmarking
{

// Sphinx start after vector
void NaiveNodeToElemMapConstruction::
  vector( ArrayView< INDEX_TYPE const, 2, 1, INDEX_TYPE, DEFAULT_BUFFER > const & elementToNodeMap,
          std::vector< std::vector< INDEX_TYPE > > & nodeToElementMap,
          INDEX_TYPE const numNodes )
{
  nodeToElementMap.resize( numNodes );

  for( INDEX_TYPE elementIndex = 0; elementIndex < elementToNodeMap.size( 0 ); ++elementIndex )
  {
    for( INDEX_TYPE const nodeIndex : elementToNodeMap[ elementIndex ] )
    {
      nodeToElementMap[ nodeIndex ].emplace_back( elementIndex );
    }
  }
}
// Sphinx end before vector

// Sphinx start after naive
void NaiveNodeToElemMapConstruction::
  naive( ArrayView< INDEX_TYPE const, 2, 1, INDEX_TYPE, DEFAULT_BUFFER > const & elementToNodeMap,
         ArrayOfArrays< INDEX_TYPE, INDEX_TYPE, DEFAULT_BUFFER > & nodeToElementMap,
         INDEX_TYPE const numNodes )
{
  nodeToElementMap.resize( numNodes );

  for( INDEX_TYPE elementIndex = 0; elementIndex < elementToNodeMap.size( 0 ); ++elementIndex )
  {
    for( INDEX_TYPE const nodeIndex : elementToNodeMap[ elementIndex ] )
    {
      nodeToElementMap.emplaceBack( nodeIndex, elementIndex );
    }
  }
}
// Sphinx end before naive

// Sphinx start after overAllocation
template< typename POLICY >
void NodeToElemMapConstruction< POLICY >::
overAllocation( ArrayView< INDEX_TYPE const, 2, 1, INDEX_TYPE, DEFAULT_BUFFER > const & elementToNodeMap,
                ArrayOfArrays< INDEX_TYPE, INDEX_TYPE, DEFAULT_BUFFER > & nodeToElementMap,
                INDEX_TYPE const numNodes,
                INDEX_TYPE const maxNodeElements )
{
  using ATOMIC_POLICY = typename RAJAHelper< POLICY >::AtomicPolicy;

  // Resize the node to element map allocating space for each inner array.
  nodeToElementMap.resize( numNodes, maxNodeElements );

  // Create an ArrayOfArraysView
  ArrayOfArraysView< INDEX_TYPE, INDEX_TYPE const, false, DEFAULT_BUFFER > const & nodeToElementMapView =
    nodeToElementMap.toView();

  // Launch a RAJA kernel that populates the ArrayOfArraysView.
  RAJA::forall< POLICY >(
    RAJA::TypedRangeSegment< INDEX_TYPE >( 0, elementToNodeMap.size( 0 ) ),
    [elementToNodeMap, nodeToElementMapView] ( INDEX_TYPE const elementIndex )
  {
    for( INDEX_TYPE const nodeIndex : elementToNodeMap[ elementIndex ] )
    {
      nodeToElementMapView.emplaceBackAtomic< ATOMIC_POLICY >( nodeIndex, elementIndex );
    }
  }
    );
}
// Sphinx end before overAllocation

// Sphinx start after resizeFromCapacities
template< typename POLICY >
void NodeToElemMapConstruction< POLICY >::
resizeFromCapacities( ArrayView< INDEX_TYPE const, 2, 1, INDEX_TYPE, DEFAULT_BUFFER > const & elementToNodeMap,
                      ArrayOfArrays< INDEX_TYPE, INDEX_TYPE, DEFAULT_BUFFER > & nodeToElementMap,
                      INDEX_TYPE const numNodes )
{
  using ATOMIC_POLICY = typename RAJAHelper< POLICY >::AtomicPolicy;

  // Create an Array containing the size of each inner array.
  Array< INDEX_TYPE, 1, RAJA::PERM_I, INDEX_TYPE, DEFAULT_BUFFER > elementsPerNode( numNodes );

  // Calculate the size of each inner array.
  RAJA::forall< POLICY >(
    RAJA::TypedRangeSegment< INDEX_TYPE >( 0, elementToNodeMap.size( 0 ) ),
    [elementToNodeMap, &elementsPerNode] ( INDEX_TYPE const elementIndex )
  {
    for( INDEX_TYPE const nodeIndex : elementToNodeMap[ elementIndex ] )
    {
      RAJA::atomicInc< ATOMIC_POLICY >( &elementsPerNode[ nodeIndex ] );
    }
  }
    );

  // Resize the node to element map with the inner array sizes.
  nodeToElementMap.resizeFromCapacities< POLICY >( elementsPerNode.size(), elementsPerNode.data() );

  // Create an ArrayOfArraysView
  ArrayOfArraysView< INDEX_TYPE, INDEX_TYPE const, false, DEFAULT_BUFFER > const & nodeToElementMapView =
    nodeToElementMap.toView();

  // Launch a RAJA kernel that populates the ArrayOfArraysView.
  RAJA::forall< POLICY >(
    RAJA::TypedRangeSegment< INDEX_TYPE >( 0, elementToNodeMap.size( 0 ) ),
    [elementToNodeMap, nodeToElementMapView] ( INDEX_TYPE const elementIndex )
  {
    for( INDEX_TYPE const nodeIndex : elementToNodeMap[ elementIndex ] )
    {
      nodeToElementMapView.emplaceBackAtomic< ATOMIC_POLICY >( nodeIndex, elementIndex );
    }
  } );
}
// Sphinx end before resizeFromCapacities

// Explicit instantiation of NodeToElemMapConstruction.
template class NodeToElemMapConstruction< serialPolicy >;

#if defined(USE_OPENMP)
template class NodeToElemMapConstruction< parallelHostPolicy >;
#endif

} // namespace benchmarking
} // namespace LvArray
