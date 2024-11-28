#pragma once

#define EXPECT_STDEXCEPT(statement, expected_exception, expected_what)         \
	EXPECT_THROW(                                                              \
	    {                                                                      \
		    try {                                                              \
			    statement;                                                     \
		    } catch (const expected_exception &e) {                            \
			    EXPECT_STREQ(e.what(), expected_what);                         \
			    throw;                                                         \
		    }                                                                  \
	    },                                                                     \
	    expected_exception                                                     \
	)
