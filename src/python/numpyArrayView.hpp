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
 * @file numpyArrayView.hpp
 */

#pragma once
#include "numpyConversion.hpp"
#include "../ArrayView.hpp"

namespace LvArray
{

namespace python
{

/**
 * Return a Numpy view of an Array
 * @param arr the ArrayView to convert to numpy.
 */
template< typename T, int NDIM, int USD, typename INDEX_TYPE, template<typename> class BUFFER_TYPE >
PyObject * create( ArrayView<T, NDIM, USD, INDEX_TYPE, BUFFER_TYPE > const & arr )
{
    arr.move( MemorySpace::CPU );
    return internal::create( arr.data(), NDIM, arr.dims(), arr.strides() );
}

} // namespace python

} // namespace LvArray
