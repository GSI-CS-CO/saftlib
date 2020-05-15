#ifndef TESTPLUGIN_SUPER_H_
#define TESTPLUGIN_SUPER_H_

namespace testplugin
{

	class Super
	{
	public:
		// saftbus property
		int getValue() const;
		// saftbus property
		void setValue(int Value);
	private:
		int Value;
	};


}


#endif
