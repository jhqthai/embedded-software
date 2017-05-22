/*! @file
 *
 *  @brief Implementation of Median filter.
 *
 *  This contains the function definitions for performing a median filter on byte-sized data.
 *
 *  @author John Thai, Jason Gavriel
 *  @date 2017-05-16
 */
/*!
**  @addtogroup Median_module I2C module documentation
**  @{
*/
/* MODULE Median */

#include "median.h"


uint8_t Median_Filter3(const uint8_t n1, const uint8_t n2, const uint8_t n3)
{
	if (n1>n2)
	{
		if (n2>n3)
			return n2;
		if (n3>n1)
			return n1;
		else
			return n3;
	}
	else
	{
		if (n1>n3)
			return n1;
		if (n3>n2)
			return n2;
		else
			return n3;
	}
}

/*!
 * @}
*/




