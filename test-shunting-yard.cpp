#include <iostream>
#include <memory>
#include <string>
#include "catch.hpp"

#include "./shunting-yard.h"

TokenMap vars, emap, tmap, key3;

void PREPARE_ENVIRONMENT() {
  vars["pi"] = 3.14;
  vars["b1"] = 0.0;
  vars["b2"] = 0.86;
  vars["_b"] = 0;
  vars["str1"] = "foo";
  vars["str2"] = "bar";
  vars["str3"] = "foobar";
  vars["str4"] = "foo10";
  vars["str5"] = "10bar";

  vars["map"] = tmap;
  tmap["key"] = "mapped value";
  tmap["key1"] = "second mapped value";
  tmap["key2"] = 10;
  tmap["key3"] = key3;
  tmap["key3"]["map1"] = "inception1";
  tmap["key3"]["map2"] = "inception2";

  emap["a"] = 10;
  emap["b"] = 20;
}

TEST_CASE("Static calculate::calculate()", "[calculate]") {
  REQUIRE(calculator::calculate("-pi + 1", vars).asDouble() == Approx(-2.14));
  REQUIRE(calculator::calculate("-pi + 1 * b1", vars).asDouble() == Approx(-3.14));
  REQUIRE(calculator::calculate("(20+10)*3/2-3", vars).asDouble() == Approx(42.0));
  REQUIRE(calculator::calculate("1 << 4", vars).asDouble() == Approx(16.0));
  REQUIRE(calculator::calculate("1+(-2*3)", vars).asDouble() == Approx(-5));
  REQUIRE(calculator::calculate("1+_b+(-2*3)", vars).asDouble() == Approx(-5));
  REQUIRE(calculator::calculate("4 * -3", vars).asInt() == -12);
}

TEST_CASE("calculate::compile() and calculate::eval()", "[compile]") {
  calculator c1;
  c1.compile("-pi+1", vars);
  REQUIRE(c1.eval().asDouble() == Approx(-2.14));

  calculator c2("pi+4", vars);
  REQUIRE(c2.eval().asDouble() == Approx(7.14));
  REQUIRE(c2.eval().asDouble() == Approx(7.14));

  calculator c3("pi+b1+b2", vars);
  REQUIRE(c3.eval(vars).asDouble() == Approx(4.0));
}

TEST_CASE("Boolean expressions") {
  REQUIRE_FALSE(calculator::calculate("3 < 3").asBool());
  REQUIRE(calculator::calculate("3 <= 3").asBool());
  REQUIRE_FALSE(calculator::calculate("3 > 3").asBool());
  REQUIRE(calculator::calculate("3 >= 3").asBool());
  REQUIRE(calculator::calculate("3 == 3").asBool());
  REQUIRE_FALSE(calculator::calculate("3 != 3").asBool());

  REQUIRE(calculator::calculate("(3 && True) == True").asBool());
  REQUIRE_FALSE(calculator::calculate("(3 && 0) == True").asBool());
  REQUIRE(calculator::calculate("(3 || 0) == True").asBool());
  REQUIRE_FALSE(calculator::calculate("(False || 0) == True").asBool());

  REQUIRE_FALSE(calculator::calculate("10 == None").asBool());
  REQUIRE(calculator::calculate("10 != None").asBool());
  REQUIRE_FALSE(calculator::calculate("10 == 'str'").asBool());
  REQUIRE(calculator::calculate("10 != 'str'").asBool());

  REQUIRE(calculator::calculate("True")->type == BOOL);
  REQUIRE(calculator::calculate("False")->type == BOOL);
  REQUIRE(calculator::calculate("10 == 'str'")->type == BOOL);
  REQUIRE(calculator::calculate("10 == 10")->type == BOOL);
}

TEST_CASE("String expressions") {
  REQUIRE(calculator::calculate("str1 + str2 == str3", vars).asBool());
  REQUIRE_FALSE(calculator::calculate("str1 + str2 != str3", vars).asBool());
  REQUIRE(calculator::calculate("str1 + 10 == str4", vars).asBool());
  REQUIRE(calculator::calculate("10 + str2 == str5", vars).asBool());

  REQUIRE(calculator::calculate("'foo' + \"bar\" == str3", vars).asBool());
  REQUIRE(calculator::calculate("'foo' + \"bar\" != 'foobar\"'", vars).asBool());

  // Test escaping characters:
  REQUIRE(calculator::calculate("'foo\\'bar'").asString() == "foo'bar");
  REQUIRE(calculator::calculate("\"foo\\\"bar\"").asString() == "foo\"bar");

  // Special meaning escaped characters:
  REQUIRE(calculator::calculate("'foo\\bar'").asString() == "foo\\bar");
  REQUIRE(calculator::calculate("'foo\\nar'").asString() == "foo\nar");
  REQUIRE(calculator::calculate("'foo\\tar'").asString() == "foo\tar");
  REQUIRE_NOTHROW(calculator::calculate("'foo\\t'"));
  REQUIRE(calculator::calculate("'foo\\t'").asString() == "foo\t");

  // Scaping linefeed:
  REQUIRE_THROWS(calculator::calculate("'foo\nar'"));
  REQUIRE(calculator::calculate("'foo\\\nar'").asString() == "foo\nar");
}

TEST_CASE("Testing operator parsing mechanism", "[operator]") {
  calculator c1;

  // Test if it is detecting operations correctly:
  REQUIRE_NOTHROW(c1.compile("['list'] == ['list']"));
  REQUIRE(c1.eval() == true);

  REQUIRE_NOTHROW(c1.compile("['list']== ['list']"));
  REQUIRE(c1.eval() == true);

  REQUIRE_NOTHROW(c1.compile("['list'] ==['list']"));
  REQUIRE(c1.eval() == true);

  REQUIRE_NOTHROW(c1.compile("['list']==['list']"));
  REQUIRE(c1.eval() == true);

  REQUIRE_NOTHROW(c1.compile("{a:'list'} == {a:'list'}"));
  REQUIRE(c1.eval() == true);

  REQUIRE_NOTHROW(c1.compile("{a:'list'}== {a:'list'}"));
  REQUIRE(c1.eval() == true);

  REQUIRE_NOTHROW(c1.compile("{a:'list'} =={a:'list'}"));
  REQUIRE(c1.eval() == true);

  REQUIRE_NOTHROW(c1.compile("{a:'list'}=={a:'list'}"));
  REQUIRE(c1.eval() == true);
}

struct Test;
struct TestData_t {
  Test* t;
  TestData_t() : t(0) {}
  TestData_t(const Test& t);
  ~TestData_t();
};

struct Test : public Container<TestData_t> {
  Test() {}
  void set(Test t) { ref->t = new Test(t); }
  Test* get() { return ref->t; }

  std::weak_ptr<TestData_t> wkref() { return ref; }
  void reset() { ref.reset(); }
};

TestData_t::TestData_t(const Test& t) : t(new Test(t)) {}
TestData_t::~TestData_t() { delete t; }

TEST_CASE("Reference counting system", "[rc]") {
  SECTION("Testing constructors:") {
    Test t1;
    Test t2;
    t2.set(t1);

    REQUIRE(t1.get() == 0);
    REQUIRE(*(t2.get()) == t1);
  }
  // t1 and t2 should have been deleted by now.
  // If no exceptions were thrown it is working.

  SECTION("Testing cycles") {
    std::weak_ptr<TestData_t> r1, r2, r3, r4;
    {
      Test t1;
      Test t2;
      t2.set(t1);

      // Build a cycle:
      REQUIRE_NOTHROW(t1.set(t2));

      // Add some non cyclic references:
      Test t3;
      Test t4;
      t4.set(t2);

      // Save some weak refs for later tests:
      r1 = t1.wkref(); r2 = t2.wkref();
      r3 = t3.wkref(); r4 = t4.wkref();
    }

    CHECK(r1.expired() == false);
    CHECK(r2.expired() == false);
    CHECK(r3.expired() == true);
    CHECK(r4.expired() == true);
    REQUIRE_NOTHROW(r1.lock()->t->reset());
    CHECK(r1.expired() == true);
    CHECK(r2.expired() == true);
  }
  // t1, t2, t3 and t4 should have been deleted by now.
  // If no exceptions were thrown it is working.

  // Note:
  // There should be no memory leaks and no "still reachable"
  // blocks when testing with valgrind.
}

TEST_CASE("String operations") {
  // String formatting:
  REQUIRE(calculator::calculate("'the test %s working' % 'is'").asString() == "the test is working");
  REQUIRE(calculator::calculate("'the tests %s %s' % ('are', 'working')").asString() == "the tests are working");

  REQUIRE(calculator::calculate("'works %s% %s' % (100, 'now')").asString() == "works 100% now");

  REQUIRE(calculator::calculate("'escape \\%s works %s' % ('now')").asString() == "escape %s works now");

  REQUIRE_THROWS(calculator::calculate("'the tests %s' % ('are', 'working')"));
  REQUIRE_THROWS(calculator::calculate("'the tests %s %s' % ('are')"));

  // String indexing:
  REQUIRE(calculator::calculate("'foobar'[0]").asString() == "f");
  REQUIRE(calculator::calculate("'foobar'[3]").asString() == "b");
  REQUIRE(calculator::calculate("'foobar'[-1]").asString() == "r");
  REQUIRE(calculator::calculate("'foobar'[-3]").asString() == "b");
}

TEST_CASE("Map access expressions", "[map][map-access]") {
  REQUIRE(calculator::calculate("map[\"key\"]", vars).asString() == "mapped value");
  REQUIRE(calculator::calculate("map[\"key\"+1]", vars).asString() ==
          "second mapped value");
  REQUIRE(calculator::calculate("map[\"key\"+2] + 3 == 13", vars).asBool() == true);
  REQUIRE(calculator::calculate("map.key1", vars).asString() == "second mapped value");

  REQUIRE(calculator::calculate("map.key3.map1", vars).asString() == "inception1");
  REQUIRE(calculator::calculate("map.key3['map2']", vars).asString() == "inception2");
  REQUIRE(calculator::calculate("map[\"no_key\"]", vars) == packToken::None());
}

TEST_CASE("Prototypical inheritance tests") {
  TokenMap vars;
  TokenMap parent;
  TokenMap child(&parent);
  TokenMap grand_child(&child);

  vars["a"] = 0;
  vars["parent"] = parent;
  vars["child"] = child;
  vars["grand_child"] = grand_child;

  parent["a"] = 10;
  parent["b"] = 20;
  parent["c"] = 30;
  child["b"] = 21;
  child["c"] = 31;
  grand_child["c"] = 32;

  REQUIRE(calculator::calculate("grand_child.a - 10", vars).asDouble() == 0);
  REQUIRE(calculator::calculate("grand_child.b - 20", vars).asDouble() == 1);
  REQUIRE(calculator::calculate("grand_child.c - 30", vars).asDouble() == 2);

  REQUIRE_NOTHROW(calculator::calculate("grand_child.a = 12", vars));
  REQUIRE(calculator::calculate("parent.a", vars).asDouble() == 10);
  REQUIRE(calculator::calculate("child.a", vars).asDouble() == 10);
  REQUIRE(calculator::calculate("grand_child.a", vars).asDouble() == 12);
}

TEST_CASE("Map usage expressions", "[map][map-usage]") {
  TokenMap vars;
  vars["my_map"] = TokenMap();
  REQUIRE_NOTHROW(calculator::calculate("my_map['a'] = 1", vars));
  REQUIRE_NOTHROW(calculator::calculate("my_map['b'] = 2", vars));
  REQUIRE_NOTHROW(calculator::calculate("my_map['c'] = 3", vars));

  REQUIRE(vars["my_map"].str() == "{ \"a\": 1, \"b\": 2, \"c\": 3 }");
  REQUIRE(calculator::calculate("my_map.len()", vars).asInt() == 3);

  REQUIRE_NOTHROW(calculator::calculate("my_map.pop('b')", vars));

  REQUIRE(vars["my_map"].str() == "{ \"a\": 1, \"c\": 3 }");
  REQUIRE(calculator::calculate("my_map.len()", vars).asDouble() == 2);

  REQUIRE_NOTHROW(calculator::calculate("default = my_map.pop('b', 3)", vars));
  REQUIRE(vars["default"].asInt() == 3);
}

TEST_CASE("List usage expressions", "[list]") {
  TokenMap vars;
  vars["my_list"] = TokenList();

  REQUIRE_NOTHROW(calculator::calculate("my_list.push(1)", vars));
  REQUIRE_NOTHROW(calculator::calculate("my_list.push(2)", vars));
  REQUIRE_NOTHROW(calculator::calculate("my_list.push(3)", vars));

  REQUIRE(vars["my_list"].str() == "[ 1, 2, 3 ]");
  REQUIRE(calculator::calculate("my_list.len()", vars).asInt() == 3);

  REQUIRE_NOTHROW(calculator::calculate("my_list.pop(1)", vars));

  REQUIRE(vars["my_list"].str() == "[ 1, 3 ]");
  REQUIRE(calculator::calculate("my_list.len()", vars).asDouble() == 2);

  REQUIRE_NOTHROW(calculator::calculate("my_list.pop()", vars));
  REQUIRE(vars["my_list"].str() == "[ 1 ]");
  REQUIRE(calculator::calculate("my_list.len()", vars).asDouble() == 1);

  vars["list"] = TokenList();
  REQUIRE_NOTHROW(calculator::calculate("list.push(4).push(5).push(6)", vars));
  REQUIRE_NOTHROW(calculator::calculate("my_list.push(2).push(3)", vars));
  REQUIRE(vars["my_list"].str() == "[ 1, 2, 3 ]");
  REQUIRE(vars["list"].str() == "[ 4, 5, 6 ]");

  REQUIRE_NOTHROW(calculator::calculate("concat = my_list + list", vars));
  REQUIRE(vars["concat"].str() == "[ 1, 2, 3, 4, 5, 6 ]");
  REQUIRE(calculator::calculate("concat.len()", vars).asDouble() == 6);

  // Reverse index like python:
  REQUIRE_NOTHROW(calculator::calculate("concat[-2] = 10", vars));
  REQUIRE_NOTHROW(calculator::calculate("concat[2] = '3'", vars));
  REQUIRE_NOTHROW(calculator::calculate("concat[3] = None", vars));
  REQUIRE(vars["concat"].str() == "[ 1, 2, \"3\", None, 10, 6 ]");

  // List index out of range:
  REQUIRE_THROWS(calculator::calculate("concat[10]", vars));
  REQUIRE_THROWS(calculator::calculate("concat[-10]", vars));
  REQUIRE_THROWS(vars["concat"].asList()[10]);
  REQUIRE_THROWS(vars["concat"].asList()[-10]);

  // Testing push and pop functions:
  TokenList L;
  REQUIRE_NOTHROW(L.push("my value"));
  REQUIRE_NOTHROW(L.push(10));
  REQUIRE_NOTHROW(L.push(TokenMap()));
  REQUIRE_NOTHROW(L.push(TokenList()));

  REQUIRE(packToken(L).str() == "[ \"my value\", 10, {}, [] ]");
  REQUIRE(L.pop().str() == "[]");
  REQUIRE(packToken(L).str() == "[ \"my value\", 10, {} ]");
}

TEST_CASE("Tuple usage expressions", "[tuple]") {
  TokenMap vars;
  calculator c;

  REQUIRE_NOTHROW(c.compile("'key':'value'"));
  STuple* t0 = static_cast<STuple*>(c.eval()->clone());
  REQUIRE(t0->type == STUPLE);
  REQUIRE(t0->list().size() == 2);
  delete t0;

  REQUIRE_NOTHROW(c.compile("1, 'key':'value', 3"));
  Tuple* t1 = static_cast<Tuple*>(c.eval()->clone());
  REQUIRE(t1->type == TUPLE);
  REQUIRE(t1->list().size() == 3);

  STuple* t2 = static_cast<STuple*>(t1->list()[1]->clone());
  REQUIRE(t2->type == STUPLE);
  REQUIRE(t2->list().size() == 2);
  delete t1;
  delete t2;

  GlobalScope global;
  REQUIRE_NOTHROW(c.compile("pow, None"));
  REQUIRE(c.eval(global).str() == "([Function: pow], None)");
}

TEST_CASE("List and map constructors usage") {
  GlobalScope vars;
  REQUIRE_NOTHROW(calculator::calculate("my_map = map()", vars));
  REQUIRE_NOTHROW(calculator::calculate("my_list = list()", vars));

  REQUIRE(vars["my_map"]->type == MAP);
  REQUIRE(vars["my_list"]->type == LIST);
  REQUIRE(calculator::calculate("my_list.len()", vars).asDouble() == 0);

  REQUIRE_NOTHROW(calculator::calculate("my_list = list(1,'2',None,map(),list('sub_list'))", vars));
  REQUIRE(vars["my_list"].str() == "[ 1, \"2\", None, {}, [ \"sub_list\" ] ]");

  // Test initialization by Iterator:
  REQUIRE_NOTHROW(calculator::calculate("my_map  = map()", vars));
  REQUIRE_NOTHROW(calculator::calculate("my_map.a = 1", vars));
  REQUIRE_NOTHROW(calculator::calculate("my_map.b = 2", vars));
  REQUIRE_NOTHROW(calculator::calculate("my_list  = list(my_map)", vars));
  REQUIRE(vars["my_list"].str() == "[ \"a\", \"b\" ]");
}

TEST_CASE("Map '{}' and list '[]' constructor usage") {
  calculator c1;
  TokenMap vars;

  REQUIRE_NOTHROW(c1.compile("{ 'a': 1 }.a"));
  REQUIRE(c1.eval().asInt() == 1);

  REQUIRE_NOTHROW(c1.compile("M = {'a': 1}"));
  REQUIRE(c1.eval().str() == "{ \"a\": 1 }");

  REQUIRE_NOTHROW(c1.compile("[ 1, 2 ].len()"));
  REQUIRE(c1.eval().asInt() == 2);

  REQUIRE_NOTHROW(c1.compile("L = [1,2]"));
  REQUIRE(c1.eval().str() == "[ 1, 2 ]");
}

TEST_CASE("Test list iterable behavior") {
  GlobalScope vars;
  REQUIRE_NOTHROW(calculator::calculate("L = list(1,2,3)", vars));
  Iterator* it;
  REQUIRE_NOTHROW(it = vars["L"].asList().getIterator());
  packToken* next;
  REQUIRE_NOTHROW(next = it->next());
  REQUIRE(next != 0);
  REQUIRE(next->asDouble() == 1);

  REQUIRE_NOTHROW(next = it->next());
  REQUIRE(next != 0);
  REQUIRE(next->asDouble() == 2);

  REQUIRE_NOTHROW(next = it->next());
  REQUIRE(next != 0);
  REQUIRE(next->asDouble() == 3);

  REQUIRE_NOTHROW(next = it->next());
  REQUIRE(next == 0);

  delete it;
}

TEST_CASE("Test map iterable behavior") {
  GlobalScope vars;
  vars["M"] = TokenMap();
  vars["M"]["a"] = 1;
  vars["M"]["b"] = 2;
  vars["M"]["c"] = 3;

  Iterator* it;
  REQUIRE_NOTHROW(it = vars["M"].asMap().getIterator());
  packToken* next;
  REQUIRE_NOTHROW(next = it->next());
  REQUIRE(next != 0);
  REQUIRE(next->asString() == "a");

  REQUIRE_NOTHROW(next = it->next());
  REQUIRE(next != 0);
  REQUIRE(next->asString() == "b");

  REQUIRE_NOTHROW(next = it->next());
  REQUIRE(next != 0);
  REQUIRE(next->asString() == "c");

  REQUIRE_NOTHROW(next = it->next());
  REQUIRE(next == 0);

  delete it;
}

TEST_CASE("Function usage expressions") {
  GlobalScope vars;
  vars["pi"] = 3.141592653589793;
  vars["a"] = -4;

  REQUIRE(calculator::calculate("sqrt(4)", vars).asDouble() == 2);
  REQUIRE(calculator::calculate("sin(pi)", vars).asDouble() == Approx(0));
  REQUIRE(calculator::calculate("cos(pi/2)", vars).asDouble() == Approx(0));
  REQUIRE(calculator::calculate("tan(pi)", vars).asDouble() == Approx(0));
  calculator c("a + sqrt(4) * 2");
  REQUIRE(c.eval(vars).asDouble() == 0);
  REQUIRE(calculator::calculate("sqrt(4-a*3) * 2", vars).asDouble() == 8);
  REQUIRE(calculator::calculate("abs(42)", vars).asDouble() == 42);
  REQUIRE(calculator::calculate("abs(-42)", vars).asDouble() == 42);

  // With more than one argument:
  REQUIRE(calculator::calculate("pow(2,2)", vars).asDouble() == 4);
  REQUIRE(calculator::calculate("pow(2,3)", vars).asDouble() == 8);
  REQUIRE(calculator::calculate("pow(2,a)", vars).asDouble() == Approx(1./16));
  REQUIRE(calculator::calculate("pow(2,a+4)", vars).asDouble() == 1);

  REQUIRE_THROWS(calculator::calculate("foo(10)"));
  REQUIRE_THROWS(calculator::calculate("foo(10),"));
  REQUIRE_NOTHROW(calculator::calculate("foo,(10)"));

  REQUIRE(TokenMap::default_global()["abs"].str() == "[Function: abs]");
  REQUIRE(calculator::calculate("1,2,3,4,5").str() == "(1, 2, 3, 4, 5)");

  REQUIRE(calculator::calculate(" float('0.1') ").asDouble() == 0.1);
  REQUIRE(calculator::calculate("float(10)").asDouble() == 10);

  vars["a"] = 0;
  REQUIRE(calculator::calculate(" eval('a = 3') ", vars).asDouble() == 3);
  REQUIRE(vars["a"] == 3);

  vars["m"] = TokenMap();
  REQUIRE_THROWS(calculator::calculate("1 + float(m) * 3", vars));
  REQUIRE_THROWS(calculator::calculate("float('not a number')"));

  REQUIRE_NOTHROW(calculator::calculate("pow(1,-10)"));
  REQUIRE_NOTHROW(calculator::calculate("pow(1,+10)"));

  vars["base"] = 2;
  c.compile("pow(base,2)", vars);
  vars["base"] = 3;
  REQUIRE(c.eval().asDouble() == 4);
  REQUIRE(c.eval(vars).asDouble() == 9);
}

TEST_CASE("Built-in extend() function") {
  GlobalScope vars;

  REQUIRE_NOTHROW(calculator::calculate("a = map()", vars));
  REQUIRE_NOTHROW(calculator::calculate("b = extend(a)", vars));
  REQUIRE_NOTHROW(calculator::calculate("a.a = 10", vars));
  REQUIRE(calculator::calculate("b.a", vars).asDouble() == 10);
  REQUIRE_NOTHROW(calculator::calculate("b.a = 20", vars));
  REQUIRE(calculator::calculate("a.a", vars).asDouble() == 10);
  REQUIRE(calculator::calculate("b.a", vars).asDouble() == 20);

  REQUIRE_NOTHROW(calculator::calculate("c = extend(b)", vars));
  REQUIRE(calculator::calculate("a.instanceof(b)", vars).asBool() == false);
  REQUIRE(calculator::calculate("a.instanceof(c)", vars).asBool() == false);
  REQUIRE(calculator::calculate("b.instanceof(a)", vars).asBool() == true);
  REQUIRE(calculator::calculate("c.instanceof(a)", vars).asBool() == true);
  REQUIRE(calculator::calculate("c.instanceof(b)", vars).asBool() == true);
}

// Used on the test case below:
packToken map_str(TokenMap scope) {
  return "custom map str";
}

TEST_CASE("Built-in str() function") {
  REQUIRE(calculator::calculate(" str(None) ").asString() == "None");
  REQUIRE(calculator::calculate(" str(10) ").asString() == "10");
  REQUIRE(calculator::calculate(" str(10.1) ").asString() == "10.1");
  REQUIRE(calculator::calculate(" str('texto') ").asString() == "texto");
  REQUIRE(calculator::calculate(" str(list(1,2,3)) ").asString() == "[ 1, 2, 3 ]");
  REQUIRE(calculator::calculate(" str(map()) ").asString() == "{}");
  REQUIRE(calculator::calculate(" str(map) ").asString() == "[Function: map]");

  vars["iterator"] = packToken(new TokenList());
  vars["iterator"]->type = IT;
  REQUIRE(calculator::calculate("str(iterator)", vars).asString() == "[Iterator]");

  TokenMap vars;
  vars["my_map"] = TokenMap();
  vars["my_map"]["__str__"] = CppFunction(&map_str, {}, "map_str");
  // Test `packToken_str()` function declared on builtin-features/functions.h:
  REQUIRE(calculator::calculate(" str(my_map) ", vars) == "custom map str");
}

TEST_CASE("Multiple argument functions") {
  GlobalScope vars;
  REQUIRE_NOTHROW(calculator::calculate("total = sum(1,2,3,4)", vars));
  REQUIRE(vars["total"].asDouble() == 10);
}

TEST_CASE("Passing keyword arguments to functions", "[function][kwargs]") {
  GlobalScope vars;
  calculator c1;
  REQUIRE_NOTHROW(c1.compile("my_map = map('a':1,'b':2)", vars));
  REQUIRE_NOTHROW(c1.eval(vars));

  TokenMap map;
  REQUIRE_NOTHROW(map = vars["my_map"].asMap());

  REQUIRE(map["a"].asInt() == 1);
  REQUIRE(map["b"].asInt() == 2);

  double result;
  REQUIRE_NOTHROW(c1.compile("result = pow(2, 'exp': 3)"));
  REQUIRE_NOTHROW(c1.eval(vars));
  REQUIRE_NOTHROW(result = vars["result"].asDouble());
  REQUIRE(result == 8.0);

  REQUIRE_NOTHROW(c1.compile("result = pow('exp': 3, 'number': 2)"));
  REQUIRE_NOTHROW(c1.eval(vars));
  REQUIRE_NOTHROW(result = vars["result"].asDouble());
  REQUIRE(result == 8.0);
}

TEST_CASE("Default functions") {
  REQUIRE(calculator::calculate("type(None)").asString() == "none");
  REQUIRE(calculator::calculate("type(10.0)").asString() == "real");
  REQUIRE(calculator::calculate("type(10)").asString() == "integer");
  REQUIRE(calculator::calculate("type(True)").asString() == "boolean");
  REQUIRE(calculator::calculate("type('str')").asString() == "string");
  REQUIRE(calculator::calculate("type(str)").asString() == "function");
  REQUIRE(calculator::calculate("type(list())").asString() == "list");
  REQUIRE(calculator::calculate("type(map())").asString() == "map");

  TokenMap vars;
  vars["mymap"] = TokenMap();
  vars["mymap"]["__type__"] = "my_type";
  REQUIRE(calculator::calculate("type(mymap)", vars).asString() == "my_type");
}

TEST_CASE("Type specific functions") {
  TokenMap vars;
  vars["s1"] = "String";
  vars["s2"] = " a b ";

  REQUIRE(calculator::calculate("s1.len()", vars).asDouble() == 6);
  REQUIRE(calculator::calculate("s1.lower()", vars).asString() == "string");
  REQUIRE(calculator::calculate("s1.upper()", vars).asString() == "STRING");
  REQUIRE(calculator::calculate("s2.strip()", vars).asString() == "a b");

  calculator c1("L = 'a, b'.split(', ')", vars);
  REQUIRE(c1.eval(vars).str() == "[ \"a\", \"b\" ]");

  calculator c2("L.join(', ')");
  REQUIRE(c2.eval(vars).asString() == "a, b");
}

TEST_CASE("Assignment expressions") {
  GlobalScope vars;
  calculator::calculate("assignment = 10", vars);

  // Assigning to an unexistent variable works.
  REQUIRE(calculator::calculate("assignment", vars).asDouble() == 10);

  // Assigning to existent variables should work as well.
  REQUIRE_NOTHROW(calculator::calculate("assignment = 20", vars));
  REQUIRE(calculator::calculate("assignment", vars).asDouble() == 20);

  // Chain assigning should work with a right-to-left order:
  REQUIRE_NOTHROW(calculator::calculate("a = b = 20", vars));
  REQUIRE_NOTHROW(calculator::calculate("a = b = c = d = 30", vars));
  REQUIRE(calculator::calculate("a == b && b == c && b == d && d == 30", vars) == true);

  REQUIRE_NOTHROW(calculator::calculate("teste='b'"));

  // The user should not be able to explicit overwrite variables
  // he did not declare. So by default he can't overwrite variables
  // on the global scope:
  REQUIRE_NOTHROW(calculator::calculate("print = 'something'", vars));
  REQUIRE(vars["print"].asString() == "something");
  REQUIRE(TokenMap::default_global()["print"].str() == "[Function: print]");

  // But it should overwrite variables
  // on non-local scopes as expected:
  TokenMap child = vars.getChild();
  REQUIRE_NOTHROW(calculator::calculate("print = 'something else'", vars));
  REQUIRE(vars["print"].asString() == "something else");
  REQUIRE(child["print"]->type == NONE);
}

TEST_CASE("Assignment expressions on maps") {
  vars["m"] = TokenMap();
  calculator::calculate("m['asn'] = 10", vars);

  // Assigning to an unexistent variable works.
  REQUIRE(calculator::calculate("m['asn']", vars).asDouble() == 10);

  // Assigning to existent variables should work as well.
  REQUIRE_NOTHROW(calculator::calculate("m['asn'] = 20", vars));
  REQUIRE(calculator::calculate("m['asn']", vars).asDouble() == 20);

  // Chain assigning should work with a right-to-left order:
  REQUIRE_NOTHROW(calculator::calculate("m.a = m.b = 20", vars));
  REQUIRE_NOTHROW(calculator::calculate("m.a = m.b = m.c = m.d = 30", vars));
  REQUIRE(calculator::calculate("m.a == m.b && m.b == m.c && m.b == m.d && m.d == 30", vars) == true);

  REQUIRE_NOTHROW(calculator::calculate("m.m = m", vars));
  REQUIRE(calculator::calculate("10 + (a = m.a = m.m.b)", vars) == 40);

  REQUIRE_NOTHROW(calculator::calculate("m.m = None", vars));
  REQUIRE(calculator::calculate("m.m", vars)->type == NONE);
}

TEST_CASE("Scope management") {
  calculator c("pi+b1+b2");
  TokenMap parent;
  parent["pi"] = 3.14;
  parent["b1"] = 0;
  parent["b2"] = 0.86;

  TokenMap child = parent.getChild();

  // Check scope extension:
  REQUIRE(c.eval(child).asDouble() == Approx(4));

  child["b2"] = 1.0;
  REQUIRE(c.eval(child).asDouble() == Approx(4.14));

  // Testing with 3 namespaces:
  TokenMap vmap = child.getChild();
  vmap["b1"] = -1.14;
  REQUIRE(c.eval(vmap).asDouble() == Approx(3.0));

  TokenMap copy = vmap;
  calculator c2("pi+b1+b2", copy);
  REQUIRE(c2.eval().asDouble() == Approx(3.0));
  REQUIRE(calculator::calculate("pi+b1+b2", copy).asDouble() == Approx(3.0));
}

// Working as a slave parser implies it will return
// a pointer to the place it has stopped parsing
// and accept a list of delimiters that should make it stop.
TEST_CASE("Parsing as slave parser") {
  const char* original_code = "a=1; b=2\n c=a+b }";
  const char* code = original_code;
  TokenMap vars;
  calculator c1, c2, c3;

  // With static function:
  REQUIRE_NOTHROW(calculator::calculate(code, vars, ";}\n", &code));
  REQUIRE(code == &(original_code[3]));
  REQUIRE(vars["a"].asDouble() == 1);

  // With constructor:
  REQUIRE_NOTHROW((c2 = calculator(++code, vars, ";}\n", &code)));
  REQUIRE(code == &(original_code[8]));

  // With compile method:
  REQUIRE_NOTHROW(c3.compile(++code, vars, ";}\n", &code));
  REQUIRE(code == &(original_code[16]));

  REQUIRE_NOTHROW(c2.eval(vars));
  REQUIRE(vars["b"] == 2);

  REQUIRE_NOTHROW(c3.eval(vars));
  REQUIRE(vars["c"] == 3);

  // Testing with delimiter between brackets of the expression:
  const char* if_code = "if ( a+(b*c) == 3 ) { ... }";
  const char* multiline = "a = (\n  1,\n  2,\n  3\n)\n print(a);";

  code = if_code;
  REQUIRE_NOTHROW(calculator::calculate(if_code+4, vars, ")", &code));
  REQUIRE(code == &(if_code[18]));

  code = multiline;
  REQUIRE_NOTHROW(calculator::calculate(multiline, vars, "\n;", &code));
  REQUIRE(code == &(multiline[21]));

  const char* error_test = "a = (;  1,;  2,; 3;)\n print(a);";
  REQUIRE_THROWS(calculator::calculate(error_test, vars, "\n;", &code));
}

// This function is for internal use only:
TEST_CASE("operation_id() function", "[op_id]") {
  #define opID(t1, t2) Operation::build_mask(t1, t2)
  REQUIRE((opID(NONE, NONE)) == 0x0000000100000001);
  REQUIRE((opID(FUNC, FUNC)) == 0x0000002000000020);
  REQUIRE((opID(FUNC, ANY_TYPE)) == 0x000000200000FFFF);
  REQUIRE((opID(FUNC, ANY_TYPE)) == 0x000000200000FFFF);
}

/* * * * * Declaring adhoc operations * * * * */

struct myCalc : public calculator {
  static Config_t& my_config() {
    static Config_t conf;
    return conf;
  }

  const Config_t Config() const { return my_config(); }

  using calculator::calculator;
};

packToken op1(const packToken& left, const packToken& right,
              evaluationData* data) {
  return calculator::Default().opMap["%"][0].exec(left, right, data);
}

packToken op2(const packToken& left, const packToken& right,
              evaluationData* data) {
  return calculator::Default().opMap[","][0].exec(left, right, data);
}

packToken op3(const packToken& left, const packToken& right,
              evaluationData* data) {
  return left.asDouble() - right.asDouble();
}

packToken op4(const packToken& left, const packToken& right,
              evaluationData* data) {
  return left.asDouble() * right.asDouble();
}

packToken slash_op(const packToken& left, const packToken& right,
                   evaluationData* data) {
  return left.asDouble() / right.asDouble();
}

packToken not_unary_op(const packToken& left, const packToken& right,
                       evaluationData* data) {
  return ~right.asInt();
}

packToken not_right_unary_op(const packToken& left, const packToken& right,
                             evaluationData* data) {
  return ~left.asInt();
}

packToken lazy_increment(const packToken& left, const packToken& right,
                         evaluationData* data) {
  std::string var_name = data->left->key.asString();

  TokenMap* map_p = data->scope.findMap(var_name);
  if (map_p == 0) {
    map_p = &data->scope;
  }

  packToken value = (*map_p)[var_name];
  (*map_p)[var_name] = value.asInt() + 1;
  return value;
}

packToken eager_increment(const packToken& left, const packToken& right,
                          evaluationData* data) {
  std::string var_name = data->right->key.asString();

  TokenMap* map_p = data->scope.findMap(var_name);
  if (map_p == 0) {
    map_p = &data->scope;
  }

  return (*map_p)[var_name] = (*map_p)[var_name].asInt() + 1;
}

packToken assign_right(const packToken& left, const packToken& right,
                       evaluationData* data) {
  std::string var_name = data->right->key.asString();

  TokenMap* map_p = data->scope.findMap(var_name);
  if (map_p == 0) {
    map_p = &data->scope;
  }

  return (*map_p)[var_name] = left;
}

packToken assign_left(const packToken& left, const packToken& right,
                      evaluationData* data) {
  std::string var_name = data->left->key.asString();

  TokenMap* map_p = data->scope.findMap(var_name);
  if (map_p == 0) {
    map_p = &data->scope;
  }

  return (*map_p)[var_name] = right;
}

void slash(const char* expr, const char** rest, rpnBuilder* data) {
  data->handle_op("*");

  // Eat the next character:
  *rest = ++expr;
}

void slash_slash(const char* expr, const char** rest, rpnBuilder* data) {
  data->handle_op("-");
}

struct myCalcStartup {
  myCalcStartup() {
    OppMap_t& opp = myCalc::my_config().opPrecedence;
    opp.add(".", 1);
    opp.add("+", 2); opp.add("*", 2);
    opp.add("/", 3);
    opp.add("<=", 4); opp.add("=>", 4);

    // This operator will evaluate from right to left:
    opp.add("-", -3);

    // Unary operators:
    opp.addUnary("$$", 2);
    opp.addUnary("~", 4);
    opp.addRightUnary("!", 1);
    opp.addRightUnary("$$", 2);
    opp.addRightUnary("~", 4);

    opMap_t& opMap = myCalc::my_config().opMap;
    opMap.add({STR, "+", TUPLE}, &op1);
    opMap.add({ANY_TYPE, ".", ANY_TYPE}, &op2);
    opMap.add({NUM, "-", NUM}, &op3);
    opMap.add({NUM, "*", NUM}, &op4);
    opMap.add({NUM, "/", NUM}, &slash_op);
    opMap.add({UNARY, "~", NUM}, &not_unary_op);
    opMap.add({NUM, "~", UNARY}, &not_right_unary_op);
    opMap.add({NUM, "!", UNARY}, &not_right_unary_op);
    opMap.add({NUM, "$$", UNARY}, &lazy_increment);
    opMap.add({UNARY, "$$", NUM}, &eager_increment);
    opMap.add({ANY_TYPE, "=>", REF}, &assign_right);
    opMap.add({REF, "<=", ANY_TYPE}, &assign_left);

    parserMap_t& parser = myCalc::my_config().parserMap;
    parser.add('/', &slash);
    parser.add("//", &slash_slash);
  }
} myCalcStartup;

/* * * * * Testing adhoc operations * * * * */

TEST_CASE("Adhoc operations", "[operation][config]") {
  myCalc c1, c2;
  const char* exp = "'Lets create %s operators%s' + ('adhoc' . '!' )";
  REQUIRE_NOTHROW(c1.compile(exp));
  REQUIRE_NOTHROW(c2 = myCalc(exp, vars, 0, 0, myCalc::my_config()));

  REQUIRE(c1.eval() == "Lets create adhoc operators!");
  REQUIRE(c2.eval() == "Lets create adhoc operators!");

  // Testing opPrecedence:
  exp = "'Lets create %s operators%s' + 'adhoc' . '!'";
  REQUIRE_NOTHROW(c1.compile(exp));
  REQUIRE(c1.eval() == "Lets create adhoc operators!");

  exp = "2 - 1 * 1";  // 2 - (1 * 1)
  REQUIRE_NOTHROW(c1.compile(exp));
  REQUIRE(c1.eval() == 1);

  // Testing op associativity:
  exp = "2 - 1";
  REQUIRE_NOTHROW(c1.compile(exp));
  REQUIRE(c1.eval() == 1);

  // Associativity right to left, i.e. 2 - (1 - 1)
  exp = "2 - 1 - 1";
  REQUIRE_NOTHROW(c1.compile(exp));
  REQUIRE(c1.eval() == 2);
}

TEST_CASE("Adhoc unary operations", "[operation][unary][config]") {
  SECTION("Left Unary Operators") {
    myCalc c1;

    // * * Using custom unary operators: * * //

    REQUIRE_NOTHROW(c1.compile("~10"));
    REQUIRE(c1.eval().asInt() == ~10l);

    REQUIRE_NOTHROW(c1.compile("2 * ~10"));
    REQUIRE(c1.eval().asInt() == 2 * ~10l);

    REQUIRE_NOTHROW(c1.compile("2 * ~10 * 3"));
    REQUIRE(c1.eval().asInt() == 2 * ~(10l*3));

    calculator c2;

    // * * Using built-in unary operators: * * //

    // Testing inside brackets:
    REQUIRE_NOTHROW(c2.compile("(2 * -10) * 3"));
    REQUIRE(c2.eval() == 2 * -10 * 3);

    REQUIRE_NOTHROW(c2.compile("2 * (-10 * 3)"));
    REQUIRE(c2.eval() == 2 * (-10 * 3));

    REQUIRE_NOTHROW(c2.compile("2 * -(10 * 3)"));
    REQUIRE(c2.eval() == 2 * -(10 * 3));

    // Testing opPrecedence:
    REQUIRE_NOTHROW(c2.compile("-10 - 2"));  // (-10) - 2
    REQUIRE(c2.eval() == -12);

    TokenMap vars;
    vars["scope_map"] = TokenMap();
    vars["scope_map"]["my_var"] = 10;

    REQUIRE_NOTHROW(c2.compile("- scope_map . my_var"));  // - (map . key2)
    REQUIRE(c2.eval(vars) == -10);
  }

  SECTION("Right unary operators") {
    myCalc c1;

    // Testing with lower op precedence:
    REQUIRE_NOTHROW(c1.compile("10~"));
    REQUIRE(c1.eval().asInt() == ~10l);

    REQUIRE_NOTHROW(c1.compile("2 * 10~"));
    REQUIRE(c1.eval().asInt() == ~(2*10l));

    REQUIRE_NOTHROW(c1.compile("2 * 10~ * 3"));
    REQUIRE(c1.eval().asInt() == ~(2*10l) * 3);

    // Testing with higher op precedence:
    REQUIRE_NOTHROW(c1.compile("10!"));
    REQUIRE(c1.eval().asInt() == ~10l);

    REQUIRE_NOTHROW(c1.compile("2 * 10!"));
    REQUIRE(c1.eval().asInt() == 2 * ~10l);

    REQUIRE_NOTHROW(c1.compile("2 * 10! * 3"));
    REQUIRE(c1.eval().asInt() == 2 * ~10l * 3);

    // Testing inside brackets:
    REQUIRE_NOTHROW(c1.compile("2 * (10~ * 3)"));
    REQUIRE(c1.eval().asInt() == 2 * ~10l * 3);

    REQUIRE_NOTHROW(c1.compile("(2 * 10~) * 3"));
    REQUIRE(c1.eval().asInt() == ~(2*10l) * 3);

    REQUIRE_NOTHROW(c1.compile("(2 * 10)~ * 3"));
    REQUIRE(c1.eval().asInt() == ~(2*10l) * 3);
  }
}

TEST_CASE("Adhoc reference operations", "[operation][reference][config]") {
  myCalc c1;
  TokenMap scope;

  scope["a"] = 10;
  REQUIRE_NOTHROW(c1.compile("$$ a"));
  REQUIRE(c1.eval(scope) == 11);
  REQUIRE(scope["a"] == 11);

  scope["a"] = 10;
  REQUIRE_NOTHROW(c1.compile("a $$"));
  REQUIRE(c1.eval(scope) == 10);
  REQUIRE(scope["a"] == 11);

  scope["a"] = packToken::None();
  REQUIRE_NOTHROW(c1.compile("a <= 20"));
  REQUIRE(c1.eval(scope) == 20);
  REQUIRE(scope["a"] == 20);

  scope["a"] = packToken::None();
  REQUIRE_NOTHROW(c1.compile("30 => a"));
  REQUIRE(c1.eval(scope) == 30);
  REQUIRE(scope["a"] == 30);
}

TEST_CASE("Adhoc reservedWord parsers", "[parser][config]") {
  myCalc c1;

  REQUIRE_NOTHROW(c1.compile("2 / 2"));
  REQUIRE(c1.eval().asInt() == 1);

  REQUIRE_NOTHROW(c1.compile("2 // 2"));
  REQUIRE(c1.eval().asInt() == 0);

  REQUIRE_NOTHROW(c1.compile("2 /? 2"));
  REQUIRE(c1.eval().asInt() == 4);

  REQUIRE_NOTHROW(c1.compile("2 /! 2"));
  REQUIRE(c1.eval().asInt() == 4);
}

TEST_CASE("Custom parser for operator ':'", "[parser]") {
  packToken p1;
  calculator c2;

  REQUIRE_NOTHROW(c2.compile("{ a : 1 }"));
  REQUIRE_NOTHROW(p1 = c2.eval());
  REQUIRE(p1["a"] == 1);

  REQUIRE_NOTHROW(c2.compile("map(a : 1, b:2, c: \"c\")"));
  REQUIRE_NOTHROW(p1 = c2.eval());
  REQUIRE(p1["a"] == 1);
  REQUIRE(p1["b"] == 2);
  REQUIRE(p1["c"] == "c");
}

TEST_CASE("Resource management") {
  calculator C1, C2("1 + 1");

  // These are likely to cause seg fault if
  // RPN copy is not handled:

  // Copy:
  REQUIRE_NOTHROW(calculator C3(C2));
  // Assignment:
  REQUIRE_NOTHROW(C1 = C2);
}

/* * * * * Testing adhoc operator parser * * * * */

TEST_CASE("Adhoc operator parser", "[operator]") {
  // Testing comments:
  REQUIRE(calculator::calculate("1 + 1 # And a comment!").asInt() == 2);
  REQUIRE(calculator::calculate("1 + 1 /*And a comment!*/").asInt() == 2);
  REQUIRE(calculator::calculate("1 /* + 1 */").asInt() == 1);
  REQUIRE(calculator::calculate("1 /* in-between */ + 1").asInt() == 2);

  REQUIRE_THROWS(calculator::calculate("1 + 1 /* Never ending comment"));

  TokenMap vars;
  const char* expr = "#12345\n - 10";
  REQUIRE_NOTHROW(calculator::calculate(expr, vars, "\n", &expr));
  REQUIRE(*expr == '\n');

  ++expr;
  REQUIRE(calculator::calculate(expr).asInt() == -10);
}

TEST_CASE("Exception management") {
  calculator ecalc1, ecalc2;
  ecalc1.compile("a+b+del", emap);
  emap["del"] = 30;

  REQUIRE_THROWS(ecalc2.compile(""));
  REQUIRE_THROWS(ecalc2.compile("      "));

  // Uninitialized calculators should eval to None:
  REQUIRE(calculator().eval().str() == "None");

  REQUIRE_THROWS(ecalc1.eval());
  REQUIRE_NOTHROW(ecalc1.eval(emap));

  emap.erase("del");
  REQUIRE_THROWS(ecalc1.eval(emap));

  emap["del"] = 0;
  emap.erase("a");
  REQUIRE_NOTHROW(ecalc1.eval(emap));

  REQUIRE_NOTHROW(calculator c5("10 + - - 10"));
  REQUIRE_THROWS(calculator c5("10 + +"));
  REQUIRE_NOTHROW(calculator c5("10 + -10"));
  REQUIRE_THROWS(calculator c5("c.[10]"));

  TokenMap v1;
  v1["map"] = TokenMap();
  // Mismatched types, no supported operators.
  REQUIRE_THROWS(calculator("map * 0").eval(v1));

  // This test attempts to cause a memory leak:
  // To see if it still works run with `make check`
  REQUIRE_THROWS(calculator::calculate("a+2*no_such_variable", vars));

  REQUIRE_THROWS(ecalc2.compile("print('hello'))"));
  REQUIRE_THROWS(ecalc2.compile("map()['hello']]"));
  REQUIRE_THROWS(ecalc2.compile("map(['hello']]"));
}

TEST_CASE("Variable UTF8 name support") {
  TokenMap v1;
  v1["n_"] = 5; // Normal name
  v1["a←"] = 5; // Name starting with ascii, but contains UTF8
  v1["łn"] = 5; // Name starting with UTF8, but also has ascii
  v1["€"] = 5;  // A single UTF8 character
  v1["\xF0\x9F\x98\x82"] = 5;  // A single UTF8 character (Happy crying emoji)
  v1["€€æøå€"] = 5; // Multiple UTF8 characters
  v1["€ð€"] = 5; // Multiple UTF8 characters
  v1["hello€ð2world€"] = 5; // Mix with multiple UTF8 characters
  // Simply using calculator::calculate
  REQUIRE(calculator::calculate("n_", v1).asInt() == 5);
  REQUIRE(calculator::calculate("n_", v1) == 5);

  REQUIRE(calculator::calculate("n_ + 5", v1).asInt() == 10);
  REQUIRE(calculator::calculate("a← + 5", v1).asInt() == 10);
  REQUIRE(calculator::calculate("łn + 5", v1).asInt() == 10);
  REQUIRE(calculator::calculate("€ + 5", v1).asInt() == 10);
  REQUIRE(calculator::calculate("€€æøå€ + 5", v1).asInt() == 10);
  REQUIRE(calculator::calculate("€ð€ + 5", v1).asInt() == 10);
  REQUIRE(calculator::calculate("hello€ð2world€ + 5", v1).asInt() == 10);
  // Using calculator.compile() -> .eval()
  calculator calc;
  calc.compile("n_ + 5", v1);
  REQUIRE(calc.eval().asInt() == 10);
  calc.compile("a← + 5", v1);
  REQUIRE(calc.eval().asInt() == 10);
  calc.compile("łn + 5", v1);
  REQUIRE(calc.eval().asInt() == 10);
  calc.compile("€ + 5", v1);
  REQUIRE(calc.eval().asInt() == 10);
  calc.compile("\xF0\x9F\x98\x82 + 5", v1);
  REQUIRE(calc.eval().asInt() == 10);
  calc.compile("€€æøå€ + 5", v1);
  REQUIRE(calc.eval().asInt() == 10);
  calc.compile("€ð€ + 5", v1);
  REQUIRE(calc.eval().asInt() == 10);
  calc.compile("hello€ð2world€ + 5", v1);
  REQUIRE(calc.eval().asInt() == 10);
}

// UTF8 tests are based off of: https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
TEST_CASE("mgk25 UTF8") {
    auto testutf8str = [](const char* str) {
        rpnBuilder::parseVar(str, &str);
    };
    CHECK_THROWS(testutf8str("\x00"));       // 2.1.1  1 byte  (U-00000000) (NULL)
    CHECK_NOTHROW(testutf8str("\x80"));  // 2.1.2  2 bytes (U-00000080)
    CHECK_NOTHROW(testutf8str("ࠀ"));       // 2.1.3  3 bytes (U-00000800)
    CHECK_NOTHROW(testutf8str("𐀀"));       // 2.1.4  4 bytes (U-00010000)
    CHECK_THROWS(testutf8str("�����"));   // 2.1.5  5 bytes (U-00200000)
    CHECK_THROWS(testutf8str("������"));  // 2.1.6  6 bytes (U-04000000)

// 2.2  Last possible sequence of a certain length

    CHECK_NOTHROW(testutf8str("")); // 2.2.1  1 byte  (U-0000007F)
    CHECK_NOTHROW(testutf8str("\xDF\xBF"));     // 2.2.2  2 bytes (U-000007FF)0xDF 0xBF
    CHECK_NOTHROW(testutf8str("\xEF\xBF\xBF"));     // 2.2.3  3 bytes (U-0000FFFF)
    CHECK_THROWS(testutf8str("\xd8\x3f\xdf\xff"));    // 2.2.4  4 bytes (U-001FFFFF)
    CHECK_THROWS(testutf8str("�����"));   // 2.2.5  5 bytes (U-03FFFFFF)
    CHECK_THROWS(testutf8str("������"));  // 2.2.6  6 bytes (U-7FFFFFFF)

// 2.3  Other boundary conditions

    CHECK_NOTHROW(testutf8str("\xed\x9f\xbf")); // 2.3.1  U-0000D7FF = ed 9f bf
    CHECK_NOTHROW(testutf8str("")); // 2.3.2  U-008000E000 = ee 80
    CHECK_NOTHROW(testutf8str("\xef\xbf\xbd")); // 2.3.3  U-0000FFFD = ef bf bd
    CHECK_NOTHROW(testutf8str("\xf4\x8f\xbf\xbf")); // 2.3.4  U-0010FFFF = f4 8f bf bf =
    CHECK_THROWS(testutf8str("\xf4\x90\x80\x80")); // 2.3.5  U-00110000 = f4 90 80 80 =

// 3  Malformed sequences
// 3.1  Unexpected continuation bytes
// Each unexpected continuation byte should be separately signalled as a
// malformed sequence of its own.

    CHECK_THROWS(testutf8str("\x80")); // 3.1.1  First continuation byte 0x80
    CHECK_THROWS(testutf8str("\xbf")); // 3.1.2  Last  continuation byte 0xbf

    CHECK_THROWS(testutf8str("\x80\x80"));    // 3.1.3  2 continuation bytes
    CHECK_THROWS(testutf8str("\x80\x80\x80"));   // 3.1.4  3 continuation bytes
    CHECK_THROWS(testutf8str("\x80\x80\x80\x80"));    // 3.1.5  4 continuation bytes
    CHECK_THROWS(testutf8str("\x80\x80\x80\x80\x80"));   // 3.1.6  5 continuation bytes
    CHECK_THROWS(testutf8str("\x80\x80\x80\x80\x80\x80"));    // 3.1.7  6 continuation bytes
    CHECK_THROWS(testutf8str("\x80\x80\x80\x80\x80\x80\x80"));   // 3.1.8  7 continuation bytes

// 3.1.9  Sequence of all 64 possible continuation bytes (0x80-0xbf):

    CHECK_THROWS(testutf8str("����������������������������������������������������������������"));

// 3.2  Lonely start characters

    CHECK_THROWS(testutf8str("� � � � � � � � � � � � � � � �� � � � � � � � � � � � � � � � ")); // 3.2.1  All 32 first bytes of 2-byte sequences (0xc0-0xdf) each followed by a space character:
    CHECK_THROWS(testutf8str("� � � � � � � � � � � � � � � � ")); // 3.2.2  All 16 first bytes of 3-byte sequences (0xe0-0xef) each followed by a space character
    CHECK_THROWS(testutf8str("� � � � � � � � ")); // 3.2.3  All 8 first bytes of 4-byte sequences (0xf0-0xf7), each followed by a space character:
    CHECK_THROWS(testutf8str("� � � � ")); // 3.2.4  All 4 first bytes of 5-byte sequences (0xf8-0xfb), each followed by a space character:
    CHECK_THROWS(testutf8str("� � ")); // 3.2.5  All 2 first bytes of 6-byte sequences (0xfc-0xfd), each followed by a space character:
// 3.3  Sequences with last continuation byte missing

    CHECK_THROWS(testutf8str("\xC0")); // 3.3.1  2-byte sequence with last byte missing (U+0000):
    CHECK_THROWS(testutf8str("��"));  // 3.3.2  3-byte sequence with last byte missing (U+0000):
    CHECK_THROWS(testutf8str("���"));  // 3.3.3  4-byte sequence with last byte missing (U+0000):
    CHECK_THROWS(testutf8str("����"));  // 3.3.4  5-byte sequence with last byte missing (U+0000):
    CHECK_THROWS(testutf8str("�����"));  // 3.3.5  6-byte sequence with last byte missing (U+0000):
    CHECK_THROWS(testutf8str("�")); // 3.3.6  2-byte sequence with last byte missing (U-000007FF):
    CHECK_THROWS(testutf8str("�")); // 3.3.7  3-byte sequence with last byte missing (U-0000FFFF):
    CHECK_THROWS(testutf8str("���"));  // 3.3.8  4-byte sequence with last byte missing (U-001FFFFF):
    CHECK_THROWS(testutf8str("����"));  // 3.3.9  5-byte sequence with last byte missing (U-03FFFFFF):
    CHECK_THROWS(testutf8str("�����"));  // 3.3.10 6-byte sequence with last byte missing (U-7FFFFFFF):

// 3.4  Concatenation of incomplete sequences

    CHECK_THROWS(testutf8str("�����������������������������")); // All the 10 sequences of 3.3 concatenated, you should see 10 malformed sequences being signalled:

// 3.5  Impossible bytes

// The following two bytes cannot appear in a correct UTF-8 string

    CHECK_THROWS(testutf8str("\xfe")); // 3.5.1  fe =
    CHECK_THROWS(testutf8str("\xff")); // 3.5.2  ff =
    CHECK_THROWS(testutf8str("\xfe\xfe\xff\xff")); // 3.5.3  fe fe ff ff =

// 4  Overlong sequences
// 4.1  Examples of an overlong ASCII character

    CHECK_THROWS(testutf8str("\xc0\xaf")); // 4.1.1 U+002F = c0 af             =
    CHECK_THROWS(testutf8str("\xe0\x80\xaf")); // 4.1.2 U+002F = e0 80 af          =
    CHECK_THROWS(testutf8str("\xf0\x80\x80\xaf")); // 4.1.3 U+002F = f0 80 80 af       =
    CHECK_THROWS(testutf8str("\xf8\x80\x80\x80\xaf")); // 4.1.4 U+002F = f8 80 80 80 af    =
    CHECK_THROWS(testutf8str("\xfc\x80\x80\x80\x80\xaf")); // 4.1.5 U+002F = fc 80 80 80 80 af =

// 4.2  Maximum overlong sequences

    CHECK_THROWS(testutf8str("\xc1\xbf")); // 4.2.1  U-0000007F = c1 bf             =
    CHECK_THROWS(testutf8str("\xe0\x9f\xbf")); // 4.2.2  U-000007FF = e0 9f bf          =
    CHECK_THROWS(testutf8str("\xf0\x8f\xbf\xbf")); // 4.2.3  U-0000FFFF = f0 8f bf bf       =
    CHECK_THROWS(testutf8str("\xf8\x87\xbf\xbf\xbf")); // 4.2.4  U-001FFFFF = f8 87 bf bf bf    =
    CHECK_THROWS(testutf8str("\xfc\x83\xbf\xbf\xbf\xbf")); // 4.2.5  U-03FFFFFF = fc 83 bf bf bf bf =

// 4.3  Overlong representation of the NUL character

    CHECK_THROWS(testutf8str("\xc0\x80")); // 4.3.1  U+0000 = c0 80             =
    CHECK_THROWS(testutf8str("\xe0\x80\x80")); // 4.3.2  U+0000 = e0 80 80          =
    CHECK_THROWS(testutf8str("\xf0\x80\x80\x80")); // 4.3.3  U+0000 = f0 80 80 80       =
    CHECK_THROWS(testutf8str("\xf8\x80\x80\x80\x80")); // 4.3.4  U+0000 = f8 80 80 80 80    =
    CHECK_THROWS(testutf8str("\xfc\x80\x80\x80\x80\x80")); // 4.3.5  U+0000 = fc 80 80 80 80 80 =

// 5  Illegal code positions
// 5.1 Single UTF-16 surrogates

    CHECK_THROWS(testutf8str("\xed\xa0\x80")); // 5.1.1  U+D800 = ed a0 80 =
    CHECK_THROWS(testutf8str("\xed\xad\xbf")); // 5.1.2  U+DB7F = ed ad bf =
    CHECK_THROWS(testutf8str("\xed\xae\x80")); // 5.1.3  U+DB80 = ed ae 80 =
    CHECK_THROWS(testutf8str("\xed\xaf\xbf")); // 5.1.4  U+DBFF = ed af bf =
    CHECK_THROWS(testutf8str("\xed\xb0\x80")); // 5.1.5  U+DC00 = ed b0 80 =
    CHECK_THROWS(testutf8str("\xed\xbe\x80")); // 5.1.6  U+DF80 = ed be 80 =
    CHECK_THROWS(testutf8str("\xed\xbf\xbf")); // 5.1.7  U+DFFF = ed bf bf =

// 5.2 Paired UTF-16 surrogates

    CHECK_THROWS(testutf8str("\xed\xa0\x80\xed\xb0\x80")); // 5.2.1  U+D800 U+DC00 = ed a0 80 ed b0 80 =
    CHECK_THROWS(testutf8str("\xed\xa0\x80\xed\xbf\xbf")); // 5.2.2  U+D800 U+DFFF = ed a0 80 ed bf bf =
    CHECK_THROWS(testutf8str("\xed\xad\xbf\xed\xb0\x80")); // 5.2.3  U+DB7F U+DC00 = ed ad bf ed b0 80 =
    CHECK_THROWS(testutf8str("\xed\xad\xbf\xed\xbf\xbf")); // 5.2.4  U+DB7F U+DFFF = ed ad bf ed bf bf =
    CHECK_THROWS(testutf8str("\xed\xae\x80\xed\xb0\x80")); // 5.2.5  U+DB80 U+DC00 = ed ae 80 ed b0 80 =
    CHECK_THROWS(testutf8str("\xed\xae\x80\xed\xbf\xbf")); // 5.2.6  U+DB80 U+DFFF = ed ae 80 ed bf bf =
    CHECK_THROWS(testutf8str("\xed\xaf\xbf\xed\xb0\x80")); // 5.2.7  U+DBFF U+DC00 = ed af bf ed b0 80 =
    CHECK_THROWS(testutf8str("\xed\xaf\xbf\xed\xbf\xbf")); // 5.2.8  U+DBFF U+DFFF = ed af bf ed bf bf =

// 5.3 Noncharacter code positions (Maybe these should throw)
    CHECK_NOTHROW(testutf8str("\xef\xbf\xbe")); // 5.3.1  U+FFFE = ef bf be =
    CHECK_NOTHROW(testutf8str("\xef\xbf\xbf")); // 5.3.2  U+FFFF = ef bf bf =
// Other noncharacters:
    CHECK_NOTHROW(testutf8str("﷐﷑﷒﷓﷔﷕﷖﷗﷘﷙﷚﷛﷜﷝﷞﷟﷠﷡﷢﷣﷤﷥﷦﷧﷨﷩﷪﷫﷬﷭﷮﷯")); // 5.3.3  U+FDD0 .. U+FDEF =
    CHECK_NOTHROW(testutf8str("🿾🿿𯿾𯿿𿿾𿿿񏿾񏿿񟿾񟿿񯿾񯿿񿿾񿿿򏿾򏿿򟿾򟿿򯿾򯿿򿿾򿿿󏿾󏿿󟿾󟿿󯿾󯿿󿿾󿿿􏿾􏿿")); // 5.3.4  U+nFFFE U+nFFFF (for n = 1..10)

}

TEST_CASE("TTA language extensions") {
  TokenMap tm;
  // := Operator
  tm["a"] = 0;
  REQUIRE(tm["a"].asInt() == 0);
  REQUIRE(calculator::calculate("a := 5", tm).asInt() == 5);
    REQUIRE(calculator::calculate("False", tm).asBool() == false);
  REQUIRE(calculator::calculate("false", tm).asBool() == false);
  REQUIRE_NOTHROW(calculator::calculate("a := false", tm));
  REQUIRE_NOTHROW(calculator::calculate("a := False", tm));
  REQUIRE(tm["a"].asInt() == 5);
  // ! Unary Operator
  tm["b"] = false;
  REQUIRE(tm["b"].asBool() == false);
  REQUIRE(calculator::calculate("!b", tm).asBool() == true);


}
