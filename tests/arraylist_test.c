
#include <stdbool.h>
#define YUKTI_TEST_STRIP_PREFIX
#define YUKTI_TEST_IMPLEMENTATION
#include "yukti.h"

#define CARGS_IMPLEMENTATION
#include "../cargs.h"

TEST (arl, new_with_capacity)
{
    Cargs_ArrayList* nl = CARGS__arl_new_with_capacity (10, sizeof (int));
    NEQ_SCALAR (nl, NULL);
    NEQ_SCALAR (nl->buffer, NULL);
    EQ_SCALAR (nl->capacity, 10U);
    EQ_SCALAR (nl->len, 0U);

    // REQUIRED to keep memory sanitizer happy
    CARGS__arl_dealloc(nl);
    END();
}

TEST (arl, push_pop_within_capacity)
{
    Cargs_ArrayList* nl = CARGS__arl_new_with_capacity (2, sizeof (char) * 2);

    NEQ_SCALAR (CARGS__arl_push (nl, "a"), NULL);
    NEQ_SCALAR (CARGS__arl_push (nl, "b"), NULL);

    EQ_SCALAR (nl->capacity, 2U);
    EQ_SCALAR (nl->len, 2U);

    EQ_STRING ((char*)cargs_arl_pop (nl), "b");
    EQ_STRING ((char*)cargs_arl_pop (nl), "a");
    EQ_SCALAR (nl->len, 0U);

    // REQUIRED to keep memory sanitizer happy
    CARGS__arl_dealloc(nl);
    END();
}

TEST (arl, push_pop_beyond_capacity)
{
    Cargs_ArrayList* nl = CARGS__arl_new_with_capacity (2, sizeof (char) * 2);

    NEQ_SCALAR (CARGS__arl_push (nl, "a"), NULL);
    NEQ_SCALAR (CARGS__arl_push (nl, "b"), NULL);
    NEQ_SCALAR (CARGS__arl_push (nl, "c"), NULL);

    EQ_SCALAR (nl->capacity, 2 * 2U);

    NEQ_SCALAR (CARGS__arl_push (nl, "d"), NULL);
    NEQ_SCALAR (CARGS__arl_push (nl, "e"), NULL);

    EQ_SCALAR (nl->capacity, 4 * 2U);

    EQ_SCALAR (nl->len, 5U);

    EQ_STRING ((char*)cargs_arl_pop (nl), "e");
    EQ_STRING ((char*)cargs_arl_pop (nl), "d");
    EQ_STRING ((char*)cargs_arl_pop (nl), "c");
    EQ_STRING ((char*)cargs_arl_pop (nl), "b");
    EQ_STRING ((char*)cargs_arl_pop (nl), "a");
    EQ_SCALAR (nl->len, 0U);

    // REQUIRED to keep memory sanitizer happy
    CARGS__arl_dealloc(nl);
    END();
}

TEST (arl, contigous_items)
{
    Cargs_ArrayList* nl = CARGS__arl_new_with_capacity (5, sizeof (char));

    char a = 'a', b = 'b', c = 'c';
    NEQ_SCALAR (CARGS__arl_push (nl, &a), NULL);
    NEQ_SCALAR (CARGS__arl_push (nl, &b), NULL);
    NEQ_SCALAR (CARGS__arl_push (nl, &c), NULL);

    EQ_SCALAR (nl->len, 3U);

    EQ_SCALAR (((char*)nl->buffer)[0], 'a');
    EQ_SCALAR (((char*)nl->buffer)[1], 'b');
    EQ_SCALAR (((char*)nl->buffer)[2], 'c');

    // REQUIRED to keep memory sanitizer happy
    CARGS__arl_dealloc(nl);
    END();
}

void yt_reset()
{
}

int main (void)
{
    YT_INIT();
    new_with_capacity();
    push_pop_within_capacity();
    push_pop_beyond_capacity();
    contigous_items();
    YT_RETURN_WITH_REPORT();
}
