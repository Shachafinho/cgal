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
#include <CGAL/Arr_vertex_index_map.h>

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
  /* Construct the reflected arrangement from scratch */

  typedef Arrangement_on_surface_2<GeomeTraits, TopolTraits>        Arr;
  typedef Arrangement_on_surface_2<GeomeTraitsRes, TopolTraitsRes>  ArrRes;
  typedef GeomeTraits::Point_2                                      ArrPoint;
  typedef GeomeTraitsRes::Point_2                                   ArrResPoint;
  typedef GeomeTraits::X_monotone_curve_2                           ArrCurve;
  typedef GeomeTraitsRes::X_monotone_curve_2                        ArrResCurve;
  typedef CGAL::Arr_vertex_index_map<Arr>                           Arr_vertex_index_map;
  typedef CGAL::Arr_vertex_index_map<ArrRes>                        ArrRes_vertex_index_map;

  // Some type assertions (not all, but better then nothing).
  CGAL_static_assertion
  ((boost::is_convertible<typename GeomeTraits::Point_2, \
                          typename GeomeTraitsRes::Point_2>::value));
  CGAL_static_assertion
  ((boost::is_convertible<typename GeomeTraits::X_monotone_curve_2, \
                          typename GeomeTraitsRes::X_monotone_curve_2>::value));

  // The result arrangement cannot be the input arrangement.
  CGAL_precondition((void *)(&arr_res) != (void *)(&arr));
  

  // First, insert the reflected points as isolated vertices.
  for (auto vit = arr.vertices_begin(); vit != arr.vertices_end(); ++vit) {
    const typename ArrPoint& vp = vit->point();
    // TODO: Reflect the point using traits class
    typename ArrResPoint new_point(p.x() - vp.x(), p.y() - vp.y());

    arr_res.insert_in_face_interior(new_point, arr_res.unbounded_faces_begin());
  }

  // Create vertex to index mappings for both arrangements.
  typename Arr_vertex_index_map arr_vi_map(arr);
  typename ArrRes_vertex_index_map arr_res_vi_map(arr_res);

  // Insert the reflected curves, corresponding to arr's edges.
  for (auto eit = arr.edges_begin(); eit != arr.edges_end(); ++eit) {
    int source_vertex_index = arr_vi_map[eit->source().current_iterator().remove_const()];
    int target_vertex_index = arr_vi_map[eit->target().current_iterator().remove_const()];
    auto res_source_vh = arr_res_vi_map.vertex(source_vertex_index);
    auto res_target_vh = arr_res_vi_map.vertex(target_vertex_index);
    
    // TODO: Reflect the curve using traits class
    auto reflected_curve = ArrResCurve(res_source_vh->point(), res_target_vh->point());
    
    arr_res.insert_at_vertices(reflected_curve, res_source_vh, res_target_vh);
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
