Type-safe, buffer-safe printf to a C++ ostream or string
========================================================

    void oprintf( std:: ostream&, printf_format, ... );
    std::string strprintf( printf_format, ... );
    std::wstring wstrprintf( printf_format, ... );

(What's shown above are "conceptual" prototypes for these functions; the
actual implementation is much more complicated.)

These functions provide _type-safe printf_.  For example:

    oprintf( cout, "%s", 3 );    // run-time assertion: type mismatch
    oprintf( cout, "%s" );       // run-time assertion: too few arguments
    oprintf( cout, "hello", 3 ); // run-time assertion: too many arguments

Also, they provide _buffer-safe printf,_ since they write to an `ostream` or a
C++ `string`.

Finally, they can be very handy for calling a function without having to
explicitly create a string temporary. For example, in traditional code,
you might write this:

    if (errorcode != 0)
    {
        char buf[100];

        sprintf(buf, "error %d", errorcode);
        MessageBox(hwnd, buf, NULL, MB_OK);
    }

With the help of these functions, you can write the above code like this:

    if (errorcode != 0)
        MessageBox(hwnd, strprintf("error %d", errorcode), NULL, MB_OK);

This creates a temporary object on the stack of type `std::string`; formats
the specified string into it; passes string as a `(const char*)` to the
function; and then deletes the temporary object automatically.

You may notice that I didn't have to call `string::c_str()` on the return
value from the `strprintf()` call above.  That's because, although I
implemented `strprintf` so that it _appears_ to return a `std::string`,
it is actually implemented as a class of its own, which subclasses
`std::string`. The strprintf class supports an implicit cast from type
`(strprintf)` to type `(const char*)`.

Passing C++ strings as parameters
---------------------------------

By the way, as a little bonus, these functions accept C++ `string` objects as
a valid argument for a "%s" specifier.  For example:

    string s;
    oprintf(cout, "hello %s\n", s);  // "s.c_str()" is not required

Signed vs. unsigned, and int vs. long
-------------------------------------

By default, this code is lax about signed vs. unsigned values, and about int
vs. long values.  For example, by default the following code will run
successfully:

    int i;
    long l;
    unsigned u;

    oprintf(cout, "%d", l);  // l is a long, not an int!
    oprintf(cout, "%ld", i); // i is an int, not a long!
    oprintf(cout, "%u", i);  // i is signed, not unsigned!
    oprintf(cout, "%d", u);  // u is unsigned, not signed!

If you prefer that the code enforce correct sign and correct int/long
specifications in the printf format string, you can enable such behavior
by #defining `STREAMPRINTF_STRICT_SIGN` and/or `STREAMPRINTF_STRICT_INTSIZE`
before you #include streamprintf.h.
