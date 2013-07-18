
#ifndef COBC_TIME__TIME_POINT_H
#define COBC_TIME__TIME_POINT_H

#include <stdint.h>
#include "duration.h"

namespace cobc
{
	namespace time
	{
		/**
		 * Specifier for a location in the time continuum.
		 *
		 * Describes a time relative to an absolute time. The reference
		 * absolute time is implementation defined and not fixed. The
		 * class `TimeModel` is responsible for conversions between UTC
		 * and this class.
		 *
		 * Available Operations:
		 *
		 *     TimePoint + Duration  --> TimePoint
		 *     TimePoint - Duration  --> TimePoint
		 *     TimePoint - TimePoint --> Duration
		 *
		 * Unlike regular numeric types, the following operations are
		 * undefined:
		 *
		 *     Duration + TimePoint  --> Undefined
		 *     Duration - TimePoint  --> Undefined
		 *     TimePoint + TimePoint --> Undefined
		 *
		 * Resolution is microseconds.
		 *
		 * @ingroup	time
		 * @author	Fabian Greif <fabian.greif@dlr.de>
		 */
		class TimePoint
		{
			friend class TimeModel;

		public:
			typedef uint64_t Type;
			typedef int64_t SignedType;

		public:
			explicit
			TimePoint(const Type time = 0) :
					time(time)
			{
			}

			inline Duration
			operator -(const TimePoint& other) const
			{
				return Duration(time - other.time);
			}

			inline TimePoint
			operator -=(const Duration& d)
			{
				time = time - d.ticks;
				return TimePoint(time);
			}

			inline TimePoint
			operator +(const Duration& d) const
			{
				return TimePoint(time + d.ticks);
			}

			inline TimePoint
			operator +=(const Duration& d)
			{
				time = time + d.ticks;
				return TimePoint(time);
			}

			inline bool
			operator ==(const TimePoint& other) const
			{
				return (time == other.time);
			}

			inline bool
			operator !=(const TimePoint& other) const
			{
				return (time != other.time);
			}

			inline bool
			operator <(const TimePoint& other) const
			{
				return (static_cast<SignedType>(time - other.time)) < 0;
			}

			inline bool
			operator >(const TimePoint& other) const
			{
				return (static_cast<SignedType>(time - other.time)) > 0;
			}

			inline bool
			operator <=(const TimePoint& other) const
			{
				return (static_cast<SignedType>(time - other.time)) <= 0;
			}

			inline bool
			operator >=(const TimePoint& other) const
			{
				return (static_cast<SignedType>(time - other.time)) >= 0;
			}

			inline TimePoint&
			operator =(const TimePoint& other)
			{
				time = other.time;
				return *this;
			}

			// TODO remove this function
			Type
			asMicroseconds() const
			{
				return time;
			}

		private:
			Type time;
		};
	}
}

#endif // COBC_TIME__TIME_POINT_H
