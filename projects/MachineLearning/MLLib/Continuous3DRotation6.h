#pragma once

#include "dataTypes.h"

/*

Zhou et al -

"On the Continuity of Rotation Representations in Neural Networks"

*/

class Mtx33;
class Quat;

class Continuous3DRotation6
{
public:
	enum
	{
		kRowCount = 2,
		kColumnCount = 3,
		kElementCount = kRowCount*kColumnCount
	};

	Continuous3DRotation6(){}
	Continuous3DRotation6(const coord_type* from);

	void SetFrom(const Quat& quatIn);
	void SetFrom(const Mtx33& mtxIn);

	void ConvertTo(Quat* quatOut);
	void ConvertTo(Mtx33* mtxOut);

	union
    {
        struct
        {
            coord_type _00, _01, _02;
            coord_type _10, _11, _12;
        };
        coord_type	m[kRowCount][kColumnCount];
        coord_type	a[kElementCount];
    };
};
