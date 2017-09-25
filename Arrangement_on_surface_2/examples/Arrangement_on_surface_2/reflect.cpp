//! \file examples/Arrangement_on_surface_2/reflectcpp
// Reflect an arrangement.

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Arr_reflect.h>

typedef CGAL::Exact_predicates_exact_constructions_kernel       Kernel;
typedef CGAL::Arr_segment_traits_2<Kernel>                      Traits_2;
typedef Traits_2::Point_2                                       Point_2;
typedef Traits_2::X_monotone_curve_2                            Segment_2;
typedef CGAL::Arrangement_2<Traits_2>                           Arrangement_2;

int main()
{
  Arrangement_2 arr;
  insert_non_intersecting_curve(arr, Segment_2(Point_2(-1, -1), Point_2(1, -1)));
  insert_non_intersecting_curve(arr, Segment_2(Point_2(1, -1), Point_2(-1, 1)));
  insert_non_intersecting_curve(arr, Segment_2(Point_2(-1, 1), Point_2(-1, -1)));
  insert_non_intersecting_curve(arr, Segment_2(Point_2(1, 1), Point_2(3, 2)));
  insert_non_intersecting_curve(arr, Segment_2(Point_2(3, 2), Point_2(2, 3)));
  insert_non_intersecting_curve(arr, Segment_2(Point_2(2, 3), Point_2(1, 1)));

  Arrangement_2 reflected_arr;
  CGAL::reflect(arr, reflected_arr, true);
  for (const auto& h : reflected_arr.edge_handles())
    std::cout << h->curve() << std::endl;
  return 0;
}
