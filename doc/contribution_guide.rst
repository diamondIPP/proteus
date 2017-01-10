Contribution Guide
==================

Development
-----------

All development happens at the `project gitlab repository
<https://gitlab.cern.ch/unige-fei4tel/proteus>`_. Development of
new features, bugfixes, and other changes should start with opening an
issue in the issue tracker. The progress for each issue should be tracked
in a separate (user-specific) branch. Features that are ready to be
made available to all users are merged to the master branch with a
merge request. Try to organize your development into atomic commits,
i.e. change only one thing at a time to simplify review.

Coding Style
------------

Use ``UpperCamelCase`` for type names and ``lowerCamelCase`` for functions,
methods, and variables. Member variables use the same convention but should be
prefixed by ``m_...``. Static member variables should be prefixed by ``s_...``.
Enum values are named like type names. Constant should be prefixed by ``k...``.
Example identifiers would look as follows:

*   ``ComplicatedNamedClass``, ``AnotherStruct``, ``EnumType``
*   ``doSomething(...)``, ``thing.doSomething(...)``
*   ``aVariable``, ``m_aMemberVariable``
*   ``kTheConstant``, ``EnumValue``
*   ``MACRO_THAT_DOES_SOMETHING(...)``

Namespaces are named using the same rules as type names.

Use 2 spaces for indentation, with the following brace placement:

.. code-block:: cpp
    
    void doSomething()
    {
      int someThing;
      if (...) {
        ...
      } else {
        ...
      }
      for (...) {
        ...
      }
    }
    
    namespace Things {
    // use also for struct's
    class Thing {
    public:
      ...
    protected:
      ...
    private:
      double m_aMemberVariable;
    };
    } // namespace Things

A `clang-format <http://clang.llvm.org/docs/ClangFormat.html>`_
configuration file is provided to automatically format code in the
described style.

Coding Guideline
----------------

ROOT macros must be compatible with ROOT 5.34/36. The compiled code must be
compatible with C++11. A good general guideline for modern C++ usage can be
found in the `CppCoreGuidelines <https://github.com/isocpp/CppCoreGuidelines>`_.
A few other anecdotal tips are listed here.

*   Write code that is easy to understand for humans instead of code that
    you hope is easier for the compiler to optimize. It's usually not
    because a modern compiler is smarter than you.
*   Comment your code. Do *not* describe what the code does, but discuss
    your intentions and assumptions. What the code does can be figured
    out by reading the code itself; your intentions probably not.
*   Non-trivial should describe their parameters using `doxygen-style
    <http://doxygen.org>`_ comments. They should be placed in the
    headers and look like this:

    .. code-block:: cpp

        /** This is a brief descriptions that must end with a dot.
         *
         * \param[in]  meat     Amount of meat in pound
         * \param[in]  heat     Amount of heat in number of sacks of coal
         * \param[out] isCrispy Whether the meat will be crispy
         * \return Grill time in nanoseconds
         */
        int grill(double meat, double heat, bool& isCrispy);

*   Unrecoverable errors should be reported by throwing and
    exception. Always throw an object derived from
    ``std::exception``. ``std::runtime_error`` with a custom error
    message usually gets the job done.
*   Try to avoid to use naked ``new`` and ``delete``. Create an object
    directly on the stack if you intend to keep ownership or use a
    smart pointer, e.g. ``std::unique_ptr<...>``, when you will
    transfer ownership at some point.
*   Always explicitly transfer ownership of objects using a
    ``std::unique_ptr<...>`` or return by value. Returning a pointer or
    a reference to an object is fine but does *not* imply transfer of
    ownership, e.g. having to manually delete the object.

    **WARNING** This is currently not enforced consistently in the code base.
    Most functions that return a pointer to an object implicitly transfer
    owernship and expect the caller to manually delete the object when it is
    not being used anymore.

*   All bets are of for ROOT objects. ROOT does not care about you or
    your memory safety. It also might or might not kill puppies in the
    process.
*   Use ``std::vector`` instead of dynamics arrays with manual ``new``
    and ``delete``. Use ``std::array`` for fixed-size arrays.
*   Prefer composition over inheritance.
*   Use inheritance to define interfaces and make the common parent an
    abstract base class, i.e. a class without any implementation that
    contains only purely virtual methods.
*   An accessor method for an invariant of an object, i.e. something
    that is a readily available property of the object, does not
    require a ``get...()`` prefix. It's redundant. However, methods that
    require computation should be named appropriately.

    .. code-block:: cpp

        class Foo {
        public:
          int bar() const { return m_bar; }
          int calculateX() const { return 2 * m_bar + 42; }
        private:
          int m_bar;
        }

*   The ``inline`` statement is *not* needed when defining a class
    method already in the class definition. It is needed if you define
    a function or a class method already in a header file.
*   Use smaller-than relates, i.e. ``<`` or ``<=``, to check for
    inequalities. This clarifies bound checks since the range boundaries
    are written to the left and right of the argument as follows:
    
    .. code-block:: cpp
        
        (0 < x) && (x < 10)

*   Do not use ``using namespace ...;`` in a header file to avoid littering the
    global namespace. Especially not ``using namespace std;``.
*   All switch statements must have a ``default`` clause and each clause
    should end with a ``break``;
