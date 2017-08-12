namespace ArrTraits {
/*!
\ingroup PkgArrangement2ConceptsFunctionObjects
\cgalConcept

\cgalRefines Functor 

\cgalHasModel `ArrangementReflectionTraits_2::Reflect_2`

*/

class Reflect_2 {
public:

/// \name Operations
/// A model of this concept must provide:
/// @{

/*!
returns	the point `p` reflected through the origin.
*/ 
ArrTraits::Point_2 operator()(const ArrTraits::Point_2& p);

/*!
returns	the x-monotone curve `xcv` reflected through the origin.
*/
ArrTraits::X_monotone_curve_2 operator()(const ArrTraits::X_monotone_curve_2& xcv);

/// @}

}; /* end ArrTraits::Reflect_2 */

}
