// Copyright (c) 2013-06  INRIA Sophia-Antipolis (France).
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
// Author(s) : Shihao Wu, Clement Jamin, Pierre Alliez 

#ifndef CGAL_wlop_simplify_and_regularize_point_set_H
#define CGAL_wlop_simplify_and_regularize_point_set_H

#include <CGAL/Search_traits_3.h>
#include <CGAL/Orthogonal_k_neighbor_search.h>
#include <CGAL/property_map.h>
#include <CGAL/point_set_processing_assertions.h>
#include <CGAL/Timer.h>
#include <CGAL/Memory_sizer.h>

#include <iterator>
#include <set>
#include <algorithm>
#include <cmath>
#include <ctime>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/tbbmalloc_proxy.h>

#include <CGAL/Simple_cartesian.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_point_primitive.h>
#include <CGAL/Bbox_3.h>

namespace CGAL {
// ----------------------------------------------------------------------------
// Private section
// ----------------------------------------------------------------------------
/// \cond SKIP_IN_MANUAL

namespace simplify_and_regularize_internal{

/// Compute average and repulsion term, then 
/// compute and update sample point locations
/// 
/// \pre `radius > 0`
///
/// @tparam Kernel Geometric traits class.
/// @tparam Tree AABB-tree.
///
/// @return average term vector
template <typename Kernel,
          typename Tree,
          typename RandomAccessIterator>
typename Kernel::Point_3
compute_update_sample_point(
  const typename Kernel::Point_3& query, ///< 3D point to project
  Tree& original_aabb_tree,              ///< original AABB-tree
  Tree& sample_aabb_tree,                ///< sample AABB-tree
  const typename Kernel::FT radius2,     ///< neighborhood radius square
  const std::vector<typename Kernel::FT>& original_densities, ///<  
  const std::vector<typename Kernel::FT>& sample_densities, ///< 
  RandomAccessIterator original_first_iter, ///<
  RandomAccessIterator sample_first_iter ///<
)
{
  CGAL_point_set_processing_precondition(radius2 > 0);
  bool is_original_densities_empty = original_densities.empty();
  bool is_sample_densities_empty = sample_densities.empty();

  // basic geometric types
  typedef typename Kernel::Point_3 Point;
  typedef typename Kernel::Vector_3 Vector;
  typedef typename Kernel::FT FT;

  // types for AABB
  typedef Kernel::Sphere_3                                Circle;
  typedef std::vector<Point>::iterator                    Iterator;
  typedef CGAL::AABB_point_primitive<Kernel, Iterator>    Primitive;

  //Compute average term
  std::vector<typename Primitive::Id> neighbor_original_points;
  Circle sphere_query(query, radius2);

  original_aabb_tree.all_contained_primitives(sphere_query, 
                           std::back_inserter(neighbor_original_points));

  Vector average = CGAL::NULL_VECTOR; 
  FT weight = (FT)0.0, average_weight_sum = (FT)0.0;
  FT iradius16 = -(FT)4.0 / radius2;

  std::vector<typename Primitive::Id>::iterator iter;
  iter = neighbor_original_points.begin();
  for (; iter != neighbor_original_points.end(); ++iter)
  {
    const Point& np = *(*iter);
    int original_index = std::distance(original_first_iter, *iter);

    FT dist2 = CGAL::squared_distance(query, np);
    if (dist2 < 1e-10) continue;

    weight = exp(dist2 * iradius16);

    if (!is_original_densities_empty)
    {
      weight *= original_densities[original_index];
    }
    average_weight_sum += weight;
    average = average + (np - CGAL::ORIGIN) * weight;
  }

  if (neighbor_original_points.empty() || average_weight_sum < FT(1e-100))
  {
    average = query - CGAL::ORIGIN;
  }
  else
  {
    average = average / average_weight_sum; 
  }
  neighbor_original_points.clear();

  //Compute repulsion term
  std::vector<typename Primitive::Id> neighbor_sample_points;
  Circle sphere_query2(query, radius2);

  sample_aabb_tree.all_contained_primitives(sphere_query2, 
                         std::back_inserter(neighbor_sample_points));

  weight = (FT)0.0;
  FT repulsion_weight_sum = (FT)0.0;
  Vector repulsion = CGAL::NULL_VECTOR; 

  iter = neighbor_sample_points.begin();
  for(; iter != neighbor_sample_points.end(); ++iter)
  {
    Point np = *(*iter);
    int sample_index = std::distance(sample_first_iter, *iter);

    FT dist2 = CGAL::squared_distance(query, np);
    if (dist2 < 1e-10) continue;
    FT dist = std::sqrt(dist2);
    
    weight = std::exp(dist2 * iradius16) * std::pow(FT(1.0) / dist, 2);
   
    if (!is_sample_densities_empty)
    {
      weight *= sample_densities[sample_index];
    }

    Vector diff = query - np;

    repulsion_weight_sum += weight;
    repulsion = repulsion + diff * weight;
  }

  if (neighbor_sample_points.size() < 3 || repulsion_weight_sum < FT(1e-10))
  {
    repulsion = CGAL::NULL_VECTOR;
  }
  else
  {
    repulsion = repulsion / repulsion_weight_sum; 
  }
  neighbor_sample_points.clear();

  // Compute update sample point
  Point update_sample = CGAL::ORIGIN + average + FT(0.45) * repulsion;
  return update_sample;
}


/// Compute density weight for each original points,
/// according to their neighbor original points
/// 
/// \pre `k >= 2`, radius > 0
///
/// @tparam Kernel Geometric traits class.
/// @tparam Tree AABB-tree.
///
/// @return computed point
template <typename Kernel, typename Tree>
typename Kernel::FT
compute_density_weight_for_original_point(
  const typename Kernel::Point_3& query, ///< 3D point to project
  Tree& original_aabb_tree,                       ///< AABB-tree
  const typename Kernel::FT radius2       ///< neighbor radius square
)
{
  CGAL_point_set_processing_precondition(radius2 > 0);

  // basic geometric types
  typedef typename Kernel::Point_3                         Point;
  typedef typename Kernel::Vector_3                        Vector;
  typedef typename Kernel::FT                              FT;
                                                           
  // types for AABB                                        
  typedef Kernel::Sphere_3                                 Circle;
  typedef std::vector<Point>::iterator                     Iterator;
  typedef CGAL::AABB_point_primitive<Kernel, Iterator>     Primitive;
  typedef CGAL::AABB_traits<Kernel, Primitive>             Traits_AABB;
  typedef CGAL::AABB_tree<Traits_AABB>                     AABB_Tree;

  std::vector<typename Primitive::Id> neighbor_original_points;

  Circle sphere_query(query, radius2);
  original_aabb_tree.all_contained_primitives(sphere_query, 
                                std::back_inserter(neighbor_original_points));

  //Compute density weight
  FT density_weight = (FT)1.0;
  FT iradius16 = -(FT)4.0 / radius2;

  std::vector<typename Primitive::Id>::iterator iter;
  iter = neighbor_original_points.begin();
  for (; iter != neighbor_original_points.end(); iter++)
  {
    Point& np = *(*iter);
    FT dist2 = CGAL::squared_distance(query, np);
    if (dist2 < 1e-8) continue;
    
    density_weight += std::exp(dist2 * iradius16);
  }

  // output
  return FT(1.0) / density_weight;
}


/// Compute density weight for sample point,
/// according to their neighbor sample points
/// 
/// \pre `k >= 2`, radius > 0
///
/// @tparam Kernel Geometric traits class.
/// @tparam Tree AABB-tree.
///
/// @return computed point
template <typename Kernel, typename Tree>
typename Kernel::FT
compute_density_weight_for_sample_point(
  const typename Kernel::Point_3& query, ///< 3D point to project
  Tree& sample_aabb_tree,                ///< AABB-tree
  const typename Kernel::FT radius2       ///< neighbor radius square
)
{
  // basic geometric types
  typedef typename Kernel::Point_3                          Point;
  typedef typename Kernel::Vector_3                         Vector;
  typedef typename Kernel::FT                               FT;

  // types for AABB                                         
  typedef Kernel::Sphere_3                                  Circle;
  typedef std::vector<Point>::iterator                      Iterator;
  typedef CGAL::AABB_point_primitive<Kernel, Iterator>      Primitive;
  typedef CGAL::AABB_traits<Kernel, Primitive>              Traits_AABB;
  typedef CGAL::AABB_tree<Traits_AABB>                      AABB_Tree;

  std::vector<typename Primitive::Id> neighbor_sample_points;
  Circle sphere_query(query, radius2);
  sample_aabb_tree.all_contained_primitives(sphere_query, 
                                   std::back_inserter(neighbor_sample_points));

  //Compute density weight
  FT density_weight = (FT)1.0;
  FT iradius16 = -(FT)4.0 / radius2;

  std::vector<typename Primitive::Id>::iterator iter;
  iter = neighbor_sample_points.begin();
  for (; iter != neighbor_sample_points.end(); iter++)
  {
    Point& np = *(*iter);
   
    FT dist2 = CGAL::squared_distance(query, np);
    density_weight += std::exp(dist2 * iradius16);
  }
  
  return density_weight;
}

} // namespace simplify_and_regularize_internal

/// \endcond

// ----------------------------------------------------------------------------
// Public section
// ----------------------------------------------------------------------------

//=============================================================================
/// \ingroup PkgPointSetProcessing
/// This is an implementation of the Weighted Locally Optimal Projection (WLOP) simplification algorithm.
/// The WLOP simplification algorithm can produce a set of 
/// denoised, outlier-free and evenly distributed particles over the original 
/// dense point cloud. 
/// The core of the algorithm is a Weighted Locally Optimal Projection operator
/// with a density uniformization term. 
/// For more details, please refer to \cgalCite{wlop-2009}.
///
/// A parallel version of WLOP is provided and requires the executable to be 
/// linked against the <a href="http://www.threadingbuildingblocks.org">Intel TBB library</a>.
/// To control the number of threads used, the user may use the tbb::task_scheduler_init class.
/// See the <a href="http://www.threadingbuildingblocks.org/documentation">TBB documentation</a> 
/// for more details.
///
/// @tparam Concurrency_tag enables sequential versus parallel algorithm.
///                         Possible values are `Sequential_tag`
///                         and `Parallel_tag`.
/// @tparam OutputIterator Type of the output iterator. 
///         The type of the objects is `Kernel::Point_3`.
/// @tparam RandomAccessIterator Iterator over input points.
/// @tparam PointPMap is a model of `ReadablePropertyMap` 
///         with the value type of `ForwardIterator` as key and `Kernel::Point_3` as value type.
///         It can be omitted if RandomAccessIterator value_type is convertible  
///         to `Kernel::Point_3`.
/// @tparam Kernel Geometric traits class.
///      It can be omitted and deduced automatically from PointPMap's value_type.
///      `Kernel_traits` are used for deducing the kernel.

// This variant requires all parameters.
template <typename Concurrency_tag,
          typename OutputIterator,
          typename RandomAccessIterator,
          typename PointPMap,
          typename Kernel>
OutputIterator
wlop_simplify_and_regularize_point_set(
  RandomAccessIterator first,  ///< random-access iterator to the first input point.
  RandomAccessIterator beyond, ///< past-the-end iterator.
  OutputIterator output,       ///< output iterator where output points are put.
  PointPMap point_pmap,        ///< point property map.
  double select_percentage,    ///< percentage of points to retain. 
                               ///< The default value is set to 5(\%).
  double radius,               ///< spherical neighborhood radius.
                               ///< This is a key parameter that needs to be finely tuned.  
                               ///< The result will be irregular if too small, but a larger
                               ///< value will impact the runtime.
                               ///< In practice, choosing a radius such that each point has at least4 neighbors
                               ///< gives satisfactory result.
                               ///< The default value is set to `0.05 * diameter of bounding box`.
  unsigned int iter_number,    ///< number of iterations. %Default: 35.
                               ///< More iterations give a more regular result.
  bool require_uniform_sampling,///< an optional preprocessing, which will give better result
                               ///< if the distribution of the input points is highly non-uniform. 
                               ///< The default value is false. 
  const Kernel&                ///< geometric traits.
)
{
  // basic geometric types
  typedef typename Kernel::Point_3   Point;
  typedef typename Kernel::Vector_3  Vector;
  typedef typename Kernel::FT        FT;

  // types for AABB
  typedef Kernel::Sphere_3                             Circle;
  typedef std::vector<Point>::iterator                 Iterator;
  typedef CGAL::AABB_point_primitive<Kernel, Iterator> Primitive;
  typedef CGAL::AABB_traits<Kernel, Primitive>         Traits_AABB;
  typedef CGAL::AABB_tree<Traits_AABB>                 AABB_Tree;

  // precondition: at least one element in the container.
  // to fix: should have at least three distinct points
  // but this is costly to check
  CGAL_point_set_processing_precondition(first != beyond);
  CGAL_point_set_processing_precondition(select_percentage >= 0 
                                         && select_percentage <= 100);

  // Random shuffle
  std::random_shuffle (first, beyond);

  // Computes original(input) and sample points size 
  std::size_t number_of_original = std::distance(first, beyond);
  std::size_t number_of_sample = (std::size_t)(FT(number_of_original) * 
                                 (select_percentage / FT(100.0)));
  std::size_t first_index_to_sample = number_of_original - number_of_sample;

  // The first point iter of original and sample points
  RandomAccessIterator it;                             // point iterator
  RandomAccessIterator first_original_iter = first;
  RandomAccessIterator first_sample_iter = first;
  std::advance(first_sample_iter, first_index_to_sample);

  //Copy sample points
  std::vector<Point> sample_points;
  sample_points.reserve(number_of_sample);
  unsigned int i;                                     

  for(it = first_sample_iter; it != beyond; ++it)
  {
#ifdef CGAL_USE_PROPERTY_MAPS_API_V1
    sample_points.push_back(get(point_pmap, it));
#else
    sample_points.push_back(get(point_pmap, *it));
#endif
  }
  
  //compute default neighbor_radius, if no radius in
  if (radius < 0)
  {
    CGAL::Bbox_3 bbox(0, 0, 0, 0, 0, 0);
    for (RandomAccessIterator temp = first; temp != beyond; ++temp)
    {
#ifdef CGAL_USE_PROPERTY_MAPS_API_V1
      Point original_p = get(point_pmap, temp);
#else
      Point original_p = get(point_pmap, *temp);
#endif 
      bbox += original_p.bbox();
    }

    Point max_p(bbox.xmax(), bbox.ymax(), bbox.zmax());
    Point min_p(bbox.xmin(), bbox.ymin(), bbox.zmin());
    FT bbox_diameter = CGAL::squared_distance(max_p, min_p);
    radius = std::sqrt(bbox_diameter) * 0.025; // this estimation may fail

  }

  FT radius2 = radius * radius;
  CGAL_point_set_processing_precondition(radius > 0);

  // Initiate a AABB_Tree search for original points
  AABB_Tree orignal_aabb_tree(first_original_iter, beyond);

  std::vector<Point> update_sample_points(number_of_sample);
  std::vector<Point>::iterator sample_iter;
  
  // Compute original density weight for original points if user needed
  std::vector<FT> original_density_weights;

  if (require_uniform_sampling)
  {
    //parallel
#ifdef CGAL_LINKED_WITH_TBB
    if (boost::is_convertible<Concurrency_tag, Parallel_tag>::value)
    {
      original_density_weights.assign(number_of_original, FT(1.0));
      tbb::parallel_for(
        tbb::blocked_range<size_t>(0, number_of_original),
        [&](const tbb::blocked_range<size_t>& r)
      {
        for (size_t i = r.begin(); i < r.end(); ++i)
        {
          RandomAccessIterator cur = first;
          std::advance(cur, i);
          FT density = simplify_and_regularize_internal::
                  compute_density_weight_for_original_point<Kernel, AABB_Tree>
                                           (
                                           #ifdef CGAL_USE_PROPERTY_MAPS_API_V1
                                             get(point_pmap, cur),
                                           #else
                                             get(point_pmap, *cur),
                                           #endif 
                                             orignal_aabb_tree, 
                                             radius2);

          original_density_weights[i] = density;
        }
      }
      );
    }else
#endif
    {
      for (it = first_original_iter, i = 0; it != beyond ; ++it, ++i)
      {
        FT density = simplify_and_regularize_internal::
                     compute_density_weight_for_original_point<Kernel, AABB_Tree>
                                           (
                                           #ifdef CGAL_USE_PROPERTY_MAPS_API_V1
                                             get(point_pmap, it),
                                           #else
                                             get(point_pmap, *it),
                                           #endif  
                                             orignal_aabb_tree, 
                                             radius2);

        original_density_weights.push_back(density);
      }
    }
  }

  for (unsigned int iter_n = 0; iter_n < iter_number; ++iter_n)
  {
    RandomAccessIterator first_sample_iter = sample_points.begin();
    AABB_Tree sample_aabb_tree(sample_points.begin(), sample_points.end());

    // Compute sample density weight for sample points
    std::vector<FT> sample_density_weights;

    for (sample_iter = sample_points.begin();
         sample_iter != sample_points.end(); ++sample_iter)
    {
      FT density = simplify_and_regularize_internal::
                   compute_density_weight_for_sample_point<Kernel, AABB_Tree>
                   (*sample_iter, 
                    sample_aabb_tree, 
                    radius2);

      sample_density_weights.push_back(density);
    }
    

    std::vector<Point>::iterator update_iter = update_sample_points.begin();
    //parallel
#ifdef CGAL_LINKED_WITH_TBB
    if (boost::is_convertible<Concurrency_tag, Parallel_tag>::value)
    {
      tbb::parallel_for(
        tbb::blocked_range<size_t>(0, number_of_sample),
        [&](const tbb::blocked_range<size_t>& r)
        {
          for (size_t i = r.begin(); i != r.end(); ++i)
          {
              update_sample_points[i] = simplify_and_regularize_internal::
                    compute_update_sample_point<Kernel,
                                                AABB_Tree,
                                                RandomAccessIterator>
                                                (sample_points[i],
                                                 orignal_aabb_tree,
                                                 sample_aabb_tree,
                                                 radius2, 
                                                 original_density_weights,
                                                 sample_density_weights,
                                                 first_original_iter,
                                                 first_sample_iter);
          }
        }
      );
    }else
#endif
    {
      //sequential
      for (sample_iter = sample_points.begin();
        sample_iter != sample_points.end(); ++sample_iter, ++update_iter)
      {
        *update_iter = simplify_and_regularize_internal::
          compute_update_sample_point<Kernel,
                                      AABB_Tree,
                                      RandomAccessIterator>
                                      (*sample_iter,
                                       orignal_aabb_tree,
                                       sample_aabb_tree,
                                       radius2,
                                       original_density_weights,
                                       sample_density_weights,
                                       first_original_iter,
                                       first_sample_iter);
      }
    }
    
    sample_iter = sample_points.begin();
    for (update_iter = update_sample_points.begin();
         update_iter != update_sample_points.end();
         ++update_iter, ++sample_iter)
    {
      *sample_iter = *update_iter;
    }

  }

  // final output
  for(sample_iter = sample_points.begin(); 
      sample_iter != sample_points.end(); ++sample_iter)
  {
    *output++ = *sample_iter;
  }

  return output;
}

/// @cond SKIP_IN_MANUAL
// This variant deduces the kernel from the iterator type.
template <typename Concurrency_tag,
          typename OutputIterator,     
          typename RandomAccessIterator, 
          typename PointPMap>
OutputIterator 
wlop_simplify_and_regularize_point_set(
  RandomAccessIterator  first,  ///< iterator over the first input point
  RandomAccessIterator  beyond, ///< past-the-end iterator
  OutputIterator output,        ///< add back-inserter
  PointPMap point_pmap, ///< property map RandomAccessIterator  -> Point_3
  const double select_percentage,     ///< percentage of points to retain
  double neighbor_radius,       ///< size of neighbors.
  const unsigned int max_iter_number, ///< number of iterations.
  const bool require_uniform_sampling     ///< if needed to compute density 
                                      ///  to generate more rugularized result.                                 
) 
{
  typedef typename boost::property_traits<PointPMap>::value_type  Point;
  typedef typename Kernel_traits<Point>::Kernel                   Kernel;
  
  return wlop_simplify_and_regularize_point_set<Concurrency_tag>(
      first, beyond,
      output,
      point_pmap,
      point_pmap_output,
      select_percentage,
      neighbor_radius,
      max_iter_number,
      require_uniform_sampling,
      Kernel());
}
/// @endcond

/// @cond SKIP_IN_MANUAL
/// This variant creates a default point property map=Dereference_property_map.
template <typename Concurrency_tag,
          typename OutputIterator,     
          typename RandomAccessIterator,
          typename PointPMap>
OutputIterator
wlop_simplify_and_regularize_point_set(
  RandomAccessIterator first,  ///< forward iterator to the first input point.
  RandomAccessIterator beyond, ///< past-the-end iterator.
  OutputIterator output,    ///< add back-inserter
  PointPMap point_pmap,     ///< property map RandomAccessIterator  -> Point_3
  const double select_percentage = 5, ///< percentage of points to retain
  double neighbor_radius = -1, ///< size of neighbors.
  const unsigned int max_iter_number = 35, ///< number of iterations.
  const bool require_uniform_sampling = false ///< if needed to compute density   
                                          /// to generate a more uniform result. 
)
{
   return wlop_simplify_and_regularize_point_set
            <Concurrency_tag>(
            first, beyond,
            output,
            point_pmap,
            select_percentage, 
            neighbor_radius, 
            max_iter_number, 
            require_uniform_sampling);
}
/// @endcond


/// @cond SKIP_IN_MANUAL
/// This variant creates a default point property map=Dereference_property_map.
template <typename Concurrency_tag, 
          typename OutputIterator,     
          typename RandomAccessIterator >
OutputIterator
wlop_simplify_and_regularize_point_set(
  RandomAccessIterator  first,  ///< iterator to the first input point.
  RandomAccessIterator  beyond, ///< past-the-end iterator.
  OutputIterator output,        ///< add back-inserter.
  const double select_percentage = 5, ///< percentage of points to retain
  double neighbor_radius = -1,  ///< size of neighbors.
  const unsigned int max_iter_number = 35, ///< number of iterations.
  const bool require_uniform_sampling = false ///< if needed to compute density   
                                           ///to generate a more uniform result. 
)
{
  return wlop_simplify_and_regularize_point_set<Concurrency_tag>(
          first, beyond,
          output,
        #ifdef CGAL_USE_PROPERTY_MAPS_API_V1
          make_dereference_property_map(first),
        #else
          make_identity_property_map(typename std::iterator_traits
                                     <RandomAccessIterator >::
                                     value_type()),
        #endif
          select_percentage, 
          neighbor_radius, 
          max_iter_number, 
          require_uniform_sampling);
}
/// @endcond

} //namespace CGAL

#endif // CGAL_wlop_simplify_and_regularize_point_set_H
