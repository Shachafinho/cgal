// Copyright (c) 2017 Tel-Aviv University (Israel).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
//
// Author(s)     : Shachaf Ben Jakov <shachafinho@gmail.com>
//                 Efi Fogel <efif@post.tau.ac.il>

#ifndef CGAL_ARR_REFLECT_H
#define CGAL_ARR_REFLECT_H

#include <CGAL/license/Arrangement_on_surface_2.h>


/*! \file
 * Definition of the global Arr_reflect() function.
 */

#include <boost/optional/optional.hpp>

#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/Arr_accessor.h>

#include <vector>
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/type_traits.hpp>

#include <CGAL/assertions.h>

namespace CGAL {

/*!
 * Determines at compile-time whether T has a nested type named Reflect_2.
 * If so, 'value' holds true, and false otherwise.
 */
template <typename T>
struct has_Reflect_2
{
  // The following types must differ in size.
  typedef char yes[1];
  typedef char no[2];
  
  // Overloads for when U does or doesn't have the Reflect_2 functor, respectively.
  template <typename U> static yes& check(typename U::Reflect_2 *);
  template <typename U> static no& check(...);

  // True iff the first overload worked and T has a nested type named Reflect_2.
  static const bool value = sizeof(check<T>(nullptr)) == sizeof(yes);
};


/*!
 * The following overload takes place if GeomeTraitsRes support reflection.
 * It reflects the arrangement using the traits' Reflect_2 functor.
 */
template <typename GeomeTraits, typename GeomeTraitsRes,
          typename TopolTraits, typename TopolTraitsRes>
typename boost::enable_if_c<has_Reflect_2<GeomeTraitsRes>::value, void>::type
do_reflect(const Arrangement_on_surface_2<GeomeTraits, TopolTraits>& arr,
                Arrangement_on_surface_2<GeomeTraitsRes, TopolTraitsRes>& arr_res)
{
  // Copy the initial arrangement and reflect it in-place

  typedef Arrangement_on_surface_2<typename GeomeTraitsRes, typename TopolTraitsRes> Arr_res;
  typedef typename GeomeTraitsRes::Reflect_2                                         Reflect_2;
  typedef CGAL::Arr_accessor<Arr_res>                                                Arr_accessor;
  typedef typename Arr_res::Vertex_iterator                                          Vertex_iterator;
  typedef typename Arr_res::Edge_iterator                                            Edge_iterator;

  // Some type assertions (not all, but better then nothing).
  CGAL_static_assertion
  ((boost::is_convertible<typename GeomeTraits::Point_2, \
                          typename GeomeTraitsRes::Point_2>::value));
  CGAL_static_assertion
  ((boost::is_convertible<typename GeomeTraits::X_monotone_curve_2, \
                          typename GeomeTraitsRes::X_monotone_curve_2>::value));

  // The result arrangement cannot be the input arrangement.
  CGAL_precondition((void *)(&arr_res) != (void *)(&arr));
  

  // Copy the initial arrangement
  arr_res.assign(arr);

  // Obtain an accessor to modify the geometric traits
  Arr_accessor accessor(arr_res);

  // Obtain a reflect object to reflect the geometric objects
  Reflect_2 reflect_object = arr_res.geometry_traits()->reflect_2_object();


  // Reflect the point of each vertex through p
  for (Vertex_iterator vit = arr_res.vertices_begin(); vit != arr_res.vertices_end(); ++vit) {
    accessor.modify_vertex_ex(vit, reflect_object(vit->point()));
  }

  // Reflect the curves as well
  for (Edge_iterator eit = arr_res.edges_begin(); eit != arr_res.edges_end(); ++eit) {
    accessor.modify_edge_ex(eit, reflect_object(eit->curve()));
  }
}

/*!
 * The following overload takes place if GeomeTraitsRes do not support reflection.
 * Reports an appropriate error message.
 */
template <typename GeomeTraits, typename GeomeTraitsRes,
          typename TopolTraits, typename TopolTraitsRes>
typename boost::enable_if_c<!has_Reflect_2<GeomeTraitsRes>::value, void>::type
  do_reflect(const Arrangement_on_surface_2<GeomeTraits, TopolTraits>& arr,
                Arrangement_on_surface_2<GeomeTraitsRes, TopolTraitsRes>& arr_res)
{
  CGAL_error_msg("The result arrangement traits do not support reflection!");
}

/*!
 * Computes the reflection of an arrangement through the origin.
 * \param arr The arrangement to reflect.
 * \param arr_res Output: The resulting arrangement.
 */
template <typename GeomeTraits, typename GeomeTraitsRes,
          typename TopolTraits, typename TopolTraitsRes>
void reflect(const Arrangement_on_surface_2<GeomeTraits, TopolTraits>& arr,
             Arrangement_on_surface_2<GeomeTraitsRes, TopolTraitsRes>& arr_res)
{
  do_reflect(arr, arr_res);
}
} //namespace CGAL

#endif
