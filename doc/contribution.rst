Contribution Guide
==================

Development
-----------

The development happens at the `project gitlab repository
<https://gitlab.cern.ch/unige-fei4tel/judith>`_. Development of
new features, bugfixes, and other changes should start with opening an
issue in the issue tracker. The progress for each issue should be tracked
in a separate (user-specific) branch. Features that are ready to be
made available to all users are merged to the master branch with a
merge request. Try to organize your development into atomic commits,
i.e. change only one thing at a time to simplify review.

Coding Style
------------

Use ``UpperCamelCase`` for type names and ``lowerCamelCase`` for
functions, methods, and variables. Member variables use the same
convention but are prefixed with an underscore. Example identifiers
would look as follows:

*   ``ComplicatedNamedClass``, ``AnotherStruct``, ``EnumType``
*   ``doSomething(...)``, ``thing.doSomething(...)``
*   ``aVariable``, ``_aMemberVariable``
*   ``A_CONSTANT``, ``ENUM_VALUE``
*   ``MACRO_THAT_DOES_SOMETHING(...)``

Namespaces are named using the same rules as type names. Using the same style
for macros, constant, and enums is kept to be consistent with existing code. In
any case, you do not plan to use macros, right?

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
      double _aMemberVariable;
    };
    } // namespace Things

A `clang-format <http://clang.llvm.org/docs/ClangFormat.html>`_
configuration file is provided to automatically format code in the
described style.

Coding Guideline
----------------

ROOT macros must be compatible with ROOT 5.34. The compiled code must be
compatible with Scientific Linux 6.x. The default compiler on Scientific Linux
6.x is GCC 4.4 which has partial support for C++11. The available features
listed in the `GCC 4.4 C++0x support status
<https://gcc.gnu.org/gcc-4.4/cxx0x_status.html>`_ can be used where appropriate.

A good general guideline for modern C++ usage is the `CppCoreGuidelines
<https://github.com/isocpp/CppCoreGuidelines>`_ and a few other anecdotal tips
are listed here.

*   Write code that is easy to understand for humans instead of code that
    you hope is easier for the compiler to optimize. It's usually not
    because a modern compiler is smarter than you.
*   Unrecoverable errors should be reported by throwing and
    exception. Always throw an object derived from
    ``std::exception``. ``std::runtime_error`` with a custom error
    message usually gets the job done.
*   Do not use naked ``new`` and ``delete``. Either create an object
    directly on the stack or use a smart pointer,
    e.g. ``std::unique_ptr<...>``.
*   Always explicitly transfer ownership of objects using a
    `std::unique_ptr<...>` or return by value. Returning a pointer or a
    reference to an object does *not* imply transfer of ownership, e.g. having
    to manually delete the object.
    
    **WARNING** This is currently not enforced consistently in the code base.
    Most functions that return a pointer to an object implicitly transfer
    owernship and expect the caller to manually delete the object when it is
    not being used anymore.
    
*   Prefer composition over inheritance.
*   Use inheritance to define interfaces and make the common parent an
    abstract base class, i.e. a class without any implementation that
    contains only purely virtual methods.
*   An accessor method for an invariant of an object, i.e. something
    that is a readily available property of the object, does not
    require a `get...()` prefix. It's redundant. However, methods that
    require computation should be named appropriately.

    .. code-block:: cpp

        class Foo {
        public:
          int bar() const { return _bar; }
          int calculateX() const { return 2 * _bar + 42; }
        private:
          int _bar;
        }
    
*   The ``inline`` statement is *not* needed when defining a class method
    already in the class definition.
*   Use smaller-than relates, i.e. ``<`` or ``<=``, to check for
    inequalities. This clarifies bound checks since the range boundaries
    are written to the left and right of the argument as follows:
    
    .. code-block:: cpp
        
        (0 < x) && (x < 10)
