#include "adder_interface.hpp"

namespace qi
{

  class Adder1: public IAdder
  {
  public:
    int addTwo(int a, int b)
    {
      int res = a+b+value.get();
      onAdd(res);
      return res;
    }
  };

  namespace details
  {
    class Adder0
    {
    public:
      int addTwo(int a, int b)
      {
        int res = a+b+value.get();
        onAdd(res);
        return res;
      }
      qi::Signal<int> onAdd;
      qi::Property<int> value;
    };
  }
}
#include "adder0_bind.hpp"
#include "adder1_bind.hpp"