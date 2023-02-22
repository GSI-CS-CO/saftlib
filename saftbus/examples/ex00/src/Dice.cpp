#include "Dice.hpp"
#include <cstdlib>
namespace ex00 {
	void Dice::Throw() {
		int number = rand()%6+1;
		SigResult.emit(number);
		FunResult(number);
	}
}
