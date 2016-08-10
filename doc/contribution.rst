Contribution Guide
==================

Development
-----------

The development happens at the `Judith Gitlab Repository
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
would like as follows:

*   ``ComplicatedNamedClass``, ``AnotherStruct``, ``EnumType``
*   ``doSomething(...)``, ``thing.doSomething(...)``
*   ``aVariable``, ``_aMemberVariable``
*   ``A_CONSTANT``, ``ENUM_VALUE``
*   ``MACRO_THAT_DOES_SOMETHING(...)``

Using the same style for macros, constant, and enums could be
problematic, but is kept to be konsistent with historical choices. In any case,
you do not plan to use macros, right?

Use 2 spaces for indentation, with the following brace placement:

.. code-block:: cpp
    
    void doSomething()
    {
      if (...) {
        ...
      } else {
        ...
      }
      for (...) {
        ...
      }
    }
    
    // use also for struct's
    class Thing {
    public:
      ...
    protected:
      ...
    private:
      ...
    };

A `clang-format <http://clang.llvm.org/docs/ClangFormat.html>`_
configuration file is provided to automatically format code in the
described style.

Coding Guideline
----------------

ROOT macros must be compatible with ROOT 5.34. The compiled code must
be compatible with Scientific Linux 6.x. A good general guideline for
modern C++ usage is the `CppCoreGuidelines
<https://github.com/isocpp/CppCoreGuidelines>`_ and a few other
anecdotal tips are listed here.

.. note:: **TODO** Discuss semantics of pointer / object ownership

*   Write code that is easy to understand for humans instead of code that
    you hope is easier for the compiler to optimize. It's usually not
    because a modern compiler is smarter than you.
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

*   Unrecoverable errors should be reported by throwing and
    exception. Always throw an object derived from
    ``std::exception``. ``std::runtime_error`` with a custom error
    message usually gets the job done.
*   Prefer composition over inheritance
*   Use inheritance to define interfaces and make the common parent an
    abstract base class, i.e. a class without any implementation that
    contains only purely virtual methods.
*   The ``inline`` statement is *not* needed when defining a Class method
    already in the class definition.
*   Do not use naked ``new`` and ``delete``. Either create an object
    directly on the stack or use a smart pointer,
    e.g. ``std::unique_ptr<...>``.
*   Use smaller-than relates, i.e. ``<`` or ``<=``, to check for
    inequalities. This clarifies bound checks since the range boundaries
    are written to the left and right of the argument as follows:
    
    .. code-block:: cpp

        (0 < x) && (x < 10)
