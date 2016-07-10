
#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <list>
#include <string>

class Function : public TokenBase {
  static TokenMap_t initialize_functions();

 public:
  static TokenMap_t default_functions;

 public:
  packToken (*func)(const Scope*);
  unsigned nargs;
  std::string* arg_names;

  Function(packToken (*func)(const Scope*), unsigned nargs, std::string* arg_names)
           : func(func), nargs(nargs), arg_names(arg_names) { this->type = FUNC; }

  virtual TokenBase* clone() const {
    return new Function(static_cast<const Function&>(*this));
  }
};

class Tuple : public TokenBase {
 public:
  typedef std::list<TokenBase*> Tuple_t;

 public:
  Tuple_t tuple;

 public:
  Tuple() {}
  Tuple(const TokenBase* a);
  Tuple(const TokenBase* a, const TokenBase* b);
  Tuple(const Tuple& t) : tuple(copyTuple(t.tuple)) { this->type = TUPLE; }
  ~Tuple() { cleanTuple(&tuple); }

 public:
  void push_back(const TokenBase* tb);
  TokenBase* pop_front();
  unsigned size();

 private:
  Tuple_t copyTuple(const Tuple_t& t);
  void cleanTuple(Tuple_t* t);

 public:
  Tuple& operator=(const Tuple& t);

  virtual TokenBase* clone() const {
    return new Tuple(static_cast<const Tuple&>(*this));
  }
};

#endif  // FUNCTIONS_H_