/*
 *	most important code here is (c) 2000 James Marr
 *  especially the METAR parsing, code taken from BWeather,
 *  which seems to be public domain, it doesn't come with a license
 */
#ifndef WEATHER_STATUS_H
#define WEATHER_STATUS_H


#include <Locker.h>
#include <String.h>

#include "HashMap.h"
#include "HashString.h"


struct WeatherData;

class WeatherStatus : public BLocker {
public:
								WeatherStatus(const char* location);
	virtual						~WeatherStatus();

	typedef enum {
		SKY_NONE,

		SKY_CLEAR,
		SKY_BROKEN,
		SKY_SCATTERED,
		SKY_FEW,
		SKY_OVERCAST
	} sky_condition;

	typedef enum {
		PHENOMENON_NONE,
		
		PHENOMENON_DRIZZLE,
		PHENOMENON_RAIN,
		PHENOMENON_SNOW,
		PHENOMENON_SNOW_GRAINS,
		PHENOMENON_ICE_CRYSTALS,
		PHENOMENON_ICE_PELLETS,
		PHENOMENON_HAIL,
		PHENOMENON_SMALL_HAIL,
		PHENOMENON_UNKNOWN_PRECIPITATION,
		
		PHENOMENON_MIST,
		PHENOMENON_FOG,
		PHENOMENON_SMOKE,
		PHENOMENON_VOLCANIC_ASH,
		PHENOMENON_SAND,
		PHENOMENON_HAZE,
		PHENOMENON_SPRAY,
		PHENOMENON_DUST,
		
		PHENOMENON_SQUALL,
		PHENOMENON_SANDSTORM,
		PHENOMENON_DUSTSTORM,
		PHENOMENON_FUNNEL_CLOUD,
		PHENOMENON_TORNADO,
		PHENOMENON_DUST_WHIRLS
	} phenomenon;

			const char*			Location() const;

			sky_condition		SkyCondition() const;
			phenomenon			Phenomenon() const;
			float				TemperatureCelsius() const;
			float				TemperatureFahrenheit() const;

			void				ParseMetar(char* data);

			void				SetTo(const WeatherData& data);
			bigtime_t			LastUpdated() const
									{ return fLastUpdated; }
private:
			BString				fLocation;

			sky_condition		fSkyCondition;
			phenomenon			fPhenomenon;
			float				fTemperature;

			bigtime_t			fLastUpdated;
};

class WeatherStatusManager : public BLocker {
public:
	static	WeatherStatusManager* Default();


			WeatherStatus*		FetchInformationFor(const char* location);
private:
	static	int32				_FtpThreadEntry(void* cookie);
			int32				_FtpThread();
			void				_DownloadStatus(WeatherStatus* status);

								WeatherStatusManager();
								~WeatherStatusManager();

	typedef HashMap<HashString, WeatherStatus*> LocationStatusMap;

			LocationStatusMap	fLocationStatusMap;
			thread_id			fFtpThread;
			sem_id				fRunningSem;

	static	WeatherStatusManager sDefaultInstance;
};

#endif // WEATHER_STATUS_H
