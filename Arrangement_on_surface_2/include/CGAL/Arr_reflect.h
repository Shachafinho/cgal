// Copyright (c) 2005,2006,2007,2008,2009,2010,2011 Tel-Aviv University (Israel).
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
 * Computes the reflection of an arrangement through a point.
 * \param arr The arrangement to reflect.
 * \param arr_res Output: The resulting arrangement.
 * \param p The point through which the reflection is done.
 */
template <typename GeomeTraits, typename GeomeTraitsRes,
          typename TopolTraits, typename TopolTraitsRes>
void reflect(const Arrangement_on_surface_2<GeomeTraits, TopolTraits>& arr,
             Arrangement_on_surface_2<GeomeTraitsRes, TopolTraitsRes>& arr_res,
             const typename GeomeTraits::Point_2& p)
{
  /* Copy the initial arrangement and reflect it in-place */

  typedef Arrangement_on_surface_2<GeomeTraitsRes, TopolTraitsRes>  ArrRes;
  typedef GeomeTraitsRes::Point_2                                   Point;
  typedef GeomeTraitsRes::X_monotone_curve_2                        Curve;
  typedef CGAL::Arr_accessor<ArrRes>                                Accessor;

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
  typename Accessor accessor(arr_res);

  // Reflect the point of each vertex through p
  for (auto vit = arr_res.vertices_begin(); vit != arr_res.vertices_end(); ++vit) {
	  const typename Point& vp = vit->point();
	  // TODO: Reflect the point using traits class
	  typename Point reflected_point(p.x() - vp.x(), p.y() - vp.y());

	  accessor.modify_vertex_ex(vit, reflected_point);
  }

  // Reflect the curves as well
  for (auto eit = arr_res.edges_begin(); eit != arr_res.edges_end(); ++eit) {
	  typename Curve reflected_curve(eit->source()->point(), eit->target()->point());
	  accessor.modify_edge_ex(eit, reflected_curve);
  }
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
  reflect(arr, arr_res, GeomeTraits::Point_2(CGAL::ORIGIN));
}

} //namespace CGAL

#endif
