//******************************************************************************
// FILE : function_variable_exclusion.hpp
//
// LAST MODIFIED : 7 December 2004
//
// DESCRIPTION :
// One variable, A, excluding another variable, B, ie A intersect not(B).  The
// list of atomic specifiers [begin_atomic(), end_atomic()) may have repeats.
//==============================================================================
#if !defined (__FUNCTION_VARIABLE_EXCLUSION_HPP__)
#define __FUNCTION_VARIABLE_EXCLUSION_HPP__

#include "computed_variable/function_variable.hpp"

class Function_variable_exclusion : public Function_variable
//******************************************************************************
// LAST MODIFIED : 7 December 2004
//
// DESCRIPTION :
// Complement with a specified "universe" ie universe intersect not(exclusion)
//==============================================================================
{
	friend class Function_variable_iterator_representation_atomic_exclusion;
	template<class Value_type_1,class Value_type_2>
		friend bool equivalent(boost::intrusive_ptr<Value_type_1> const &,
		boost::intrusive_ptr<Value_type_2> const &);
	public:
		// constructor
		Function_variable_exclusion(const Function_variable_handle& universe,
			const Function_variable_handle& exclusion);
		Function_variable_exclusion(const Function_handle& function,
			const Function_variable_handle& universe,
			const Function_variable_handle& exclusion);
	// inherited
	public:
		Function_variable_handle clone() const;
		string_handle get_string_representation();
		Function_variable_iterator begin_atomic() const;
		Function_variable_iterator end_atomic() const;
		std::reverse_iterator<Function_variable_iterator> rbegin_atomic() const;
		std::reverse_iterator<Function_variable_iterator> rend_atomic() const;
#if defined (CIRCULAR_SMART_POINTERS)
		virtual void add_dependent_function(const Function_handle);
		virtual void remove_dependent_function(const Function_handle);
#else // defined (CIRCULAR_SMART_POINTERS)
		virtual void add_dependent_function(Function*);
		virtual void remove_dependent_function(Function*);
#endif // defined (CIRCULAR_SMART_POINTERS)
	private:
		virtual bool equality_atomic(const Function_variable_handle&) const;
	private:
		// copy constructor
		Function_variable_exclusion(const Function_variable_exclusion&);
		// assignment
		Function_variable_exclusion& operator=(const Function_variable_exclusion&);
	private:
		Function_variable_handle exclusion,universe;
};

typedef boost::intrusive_ptr<Function_variable_exclusion>
	Function_variable_exclusion_handle;

#endif /* !defined (__FUNCTION_VARIABLE_EXCLUSION_HPP__) */
