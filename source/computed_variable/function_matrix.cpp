//******************************************************************************
// FILE : function_matrix.cpp
//
// LAST MODIFIED : 14 September 2004
//
// DESCRIPTION :
//==============================================================================

#if defined (ONE_TEMPLATE_DEFINITION_IMPLEMENTED)
#include "computed_variable/function_matrix_implementation.cpp"
#else // defined (ONE_TEMPLATE_DEFINITION_IMPLEMENTED)
#include "computed_variable/function_matrix.hpp"
#endif // defined (ONE_TEMPLATE_DEFINITION_IMPLEMENTED)

template<>
boost::intrusive_ptr< Function_matrix<Scalar> >
	Function_matrix<Scalar>::solve(
	const boost::intrusive_ptr< Function_matrix<Scalar> >& rhs)
//******************************************************************************
// LAST MODIFIED : 14 September 2004
//
// DESCRIPTION :
// A x = B
// n by n  n by m  n by m
//==============================================================================
{
	boost::intrusive_ptr< Function_matrix<Scalar> > result(0);
	Function_size_type number_of_rhss,size_A;

	if (this&&(0<(size_A=number_of_rows()))&&(number_of_columns()==size_A)&&
		rhs&&(rhs->number_of_rows()==size_A)&&
		(0<(number_of_rhss=rhs->number_of_columns())))
	{
		ublas::matrix<Scalar,ublas::column_major>
			A(size_A,size_A),X(size_A,number_of_rhss);
		std::vector<int> ipiv(size_A);

		A=values;
		X=rhs->values;
		lapack::gesv(A,ipiv,X);
		result=boost::intrusive_ptr< Function_matrix<Scalar> >(
			new Function_matrix<Scalar>(X));
	}

	return (result);
}

template<>
bool Function_matrix<Scalar>::determinant(Scalar& det)
//******************************************************************************
// LAST MODIFIED : 14 September 2004
//
// DESCRIPTION :
// Zero for a non-square matrix.
//==============================================================================
{
	bool result(false);
	Function_size_type size_A;

	if (this&&(0<(size_A=number_of_rows()))&&(number_of_columns()==size_A))
	{
		Function_size_type i;
		std::vector<int> ipiv(size_A);
		ublas::matrix<Scalar,ublas::column_major> A(size_A,size_A);

		A=values;
		lapack::getrf(A,ipiv);
		result=true;
		det=1;
		for (i=0;i<size_A;i++)
		{
			if (i+1==(Function_size_type)ipiv[i])
			{
				det *= A(i,i);
			}
			else
			{
				det *= -A(i,i);
			}
		}
	}

	return (result);
}

template<>
bool Function_matrix<Scalar>::evaluate_derivative(Scalar& derivative,
	Function_variable_handle atomic_variable,
	std::list<Function_variable_handle>& atomic_independent_variables)
//******************************************************************************
// LAST MODIFIED : 1 September 2004
//
// DESCRIPTION :
//==============================================================================
{
	bool result;
	boost::intrusive_ptr< Function_variable_matrix<Scalar> >
		atomic_dependent_variable,atomic_independent_variable;

	result=false;
	if ((atomic_dependent_variable=boost::dynamic_pointer_cast<
		Function_variable_matrix<Scalar>,Function_variable>(
		atomic_variable))&&
		equivalent(Function_handle(this),atomic_dependent_variable->function())&&
		(1==atomic_dependent_variable->number_differentiable()))
	{
		result=true;
		if ((1==atomic_independent_variables.size())&&
			(atomic_independent_variable=boost::dynamic_pointer_cast<
			Function_variable_matrix<Scalar>,Function_variable>(
			atomic_independent_variables.front()))&&
			equivalent(atomic_dependent_variable,atomic_independent_variable))
		{
			derivative=1;
		}
		else
		{
			derivative=0;
		}
	}

	return (result);
}
