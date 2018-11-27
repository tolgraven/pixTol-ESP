#ifdef UNIT_TEST

#include <unity.h>
#include <field.h>

void test_field() {
    uint8_t lhs[3] = {100};
    uint8_t rhs[3] = {200};
    Field* fl = new Field(lhs, 3, 0, 1.0, true);
    Field* fr = new Field(rhs, 3);
    TEST_ASSERT_EQUAL(150, *fl->avg(fr).get(0));
    fl->set(lhs);
    TEST_ASSERT_EQUAL(255, *fl->add(fr).get(0));
    fl->set(lhs);
    TEST_ASSERT_EQUAL(0, *fl->sub(fr).get(0));
    fl->set(lhs);
    TEST_ASSERT_EQUAL(200, *fl->htp(fr).get(0));
    fl->set(lhs);
    TEST_ASSERT_EQUAL(200, *fl->ltp(fr).get(0));
    fl->set(lhs);
    TEST_ASSERT_EQUAL(100, *fl->lotp(fr).get(0));
    // TEST_ASSERT(condition);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_field);

    UNITY_END();
}

#endif
