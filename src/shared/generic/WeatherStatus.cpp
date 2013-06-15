/*
 *	most important code here is (c) 2000 James Marr
 *  especially the METAR parsing, code taken from BWeather,
 *  which seems to be public domain, it doesn't come with a license
 */
#include "WeatherStatus.h"

#include <math.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <Autolock.h>
#include <Entry.h>
#include <File.h>

#include "regex.h"


using std::nothrow;

#define TIME_RE_STR  "^([0-9]{6})Z$"
#define WIND_RE_STR  "^(([0-9]{3})|VRB)([0-9]?[0-9]{2})(G[0-9]?[0-9]{2})?KT$"
#define VIS_RE_STR   "^(([0-9]?[0-9])|(M?1/[0-9]?[0-9]))SM$"
#define CLOUD_RE_STR "^(CLR|BKN|SCT|FEW|OVC)([0-9]{3})?$"
#define TEMP_RE_STR  "^(M?[0-9][0-9])/(M?[0-9][0-9])$"
#define PRES_RE_STR  "^(A|Q)([0-9]{4})$"
#define COND_RE_STR  "^(-|\\+)?(VC|MI|BC|PR|TS|BL|SH|DR|FZ)?(DZ|RA|SN|SG|IC|PE|GR|GS|UP|BR|FG|FU|VA|SA|HZ|PY|DU|SQ|SS|DS|PO|\\+?FC)$"
#define IWIN_RE_STR "([A-Z][A-Z]Z(([0-9]{3}>[0-9]{3}-)|([0-9]{3}-))+)+([0-9]{6}-)?"

#define CONST_ALPHABET "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define CONST_DIGITS "0123456789"

#define VALUE_NONE 0xfdfd

enum QUALIFIER {
	QUALIFIER_NONE,
	
	QUALIFIER_VICINITY,
	
	QUALIFIER_LIGHT,
	QUALIFIER_MODERATE,
	QUALIFIER_HEAVY,
	QUALIFIER_SHALLOW,
	QUALIFIER_PATCHES,
	QUALIFIER_PARTIAL,
	QUALIFIER_THUNDERSTORM,
	QUALIFIER_BLOWING,
	QUALIFIER_SHOWERS,
	QUALIFIER_DRIFTING,
	QUALIFIER_FREEZING
};

enum WINDDIR {
    WIND_NONE, WIND_VARIABLE,
    WIND_N, WIND_NNE, WIND_NE, WIND_ENE,
    WIND_E, WIND_ESE, WIND_SE, WIND_SSE,
    WIND_S, WIND_SSW, WIND_SW, WIND_WSW,
    WIND_W, WIND_WNW, WIND_NW, WIND_NNW,
};

const char *sky_str[] = {
	("No Information"),
    ("Clear"),
    ("Broken clouds"),
    ("Scattered clouds"),
    ("Few clouds"),
    ("Overcast")
};

const char *cond_str[24][13] = {
/*                   NONE                         VICINITY                             LIGHT                      MODERATE                      HEAVY                      SHALLOW                      PATCHES                         PARTIAL                      THUNDERSTORM                    BLOWING                      SHOWERS                         DRIFTING                      FREEZING                      */
/*               *******************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/
/* NONE          */ {"None",                          "None",                                  "None",                        "None",                           "None",                        "None",                          "None",                             "None",                          "None",                             "None",                          "None",                             "None",                           "None",                          },
/* DRIZZLE       */ {("Drizzle"),               ("Drizzle in the vicinity"),       ("Light drizzle"),       ("Moderate drizzle"),       ("Heavy drizzle"),       ("Shallow drizzle"),       ("Patches of drizzle"),       ("Partial drizzle"),       ("Thunderstorm"),             ("Windy drizzle"),         ("Showers"),                  ("Drifting drizzle"),       ("Freezing drizzle")       },
/* RAIN          */ {("Rain "),                 ("Rain in the vicinity") ,         ("Light rain"),          ("Moderate rain"),          ("Heavy rain"),          ("Shallow rain"),          ("Patches of rain"),          ("Partial rainfall"),      ("Thunderstorm"),             ("Blowing rainfall"),      ("Rain showers"),             ("Drifting rain"),          ("Freezing rain")          },
/* SNOW          */ {("Snow"),                  ("Snow in the vicinity") ,         ("Light snow"),          ("Moderate snow"),          ("Heavy snow"),          ("Shallow snow"),          ("Patches of snow"),          ("Partial snowfali"),      ("Snowstorm"),                ("Blowing snowfall"),      ("Snow showers"),             ("Drifting snow"),          ("Freezing snow")          },
/* SNOW_GRAINS   */ {("Snow grains"),           ("Snow grains in the vicinity") ,  ("Light snow grains"),   ("Moderate snow grains"),   ("Heavy snow grains"),   ("Shallow snow grains"),   ("Patches of snow grains"),   ("Partial snow grains"),   ("Snowstorm"),                ("Blowing snow grains"),   ("Snow grain showers"),       ("Drifting snow grains"),   ("Freezing snow grains")   },
/* ICE_CRYSTALS  */ {("Ice crystals"),          ("Ice crystals in the vicinity") , ("Few ice crystals"),    ("Moderate ice crystals"),  ("Heavy ice crystals"),  "??",                        ("Patches of ice crystals"),  ("Partial ice crystals"),  ("Ice crystal storm"),        ("Blowing ice crystals"),  ("Showers of ice crystals"),  ("Drifting ice crystals"),  ("Freezing ice crystals")  },
/* ICE_PELLETS   */ {("Ice pellets"),           ("Ice pellets in the vicinity") ,  ("Few ice pellets"),     ("Moderate ice pellets"),   ("Heavy ice pellets"),   ("Shallow ice pellets"),   ("Patches of ice pellets"),   ("Partial ice pellets"),   ("Ice pellet storm"),         ("Blowing ice pellets"),   ("Showers of ice pellets"),   ("Drifting ice pellets"),   ("Freezing ice pellets")   },
/* HAIL          */ {("Hail"),                  ("Hail in the vicinity") ,         ("Light hail"),          ("Moderate hail"),          ("Heavy hail"),          ("Shallow hail"),          ("Patches of hail"),          ("Partial hail"),          ("Hailstorm"),                ("Blowing hail"),          ("Hail showers"),             ("Drifting hail"),          ("Freezing hail")          },
/* SMALL_HAIL    */ {("Small hail"),            ("Small hail in the vicinity") ,   ("Light hail"),          ("Moderate small hail"),    ("Heavy small hail"),    ("Shallow small hail"),    ("Patches of small hail"),    ("Partial small hail"),    ("Small hailstorm"),          ("Blowing small hail"),    ("Showers of small hail"),    ("Drifting small hail"),    ("Freezing small hail")    },
/* PRECIPITATION */ {("Unknown precipitation"), ("Precipitation in the vicinity"), ("Light precipitation"), ("Moderate precipitation"), ("Heavy precipitation"), ("Shallow precipitation"), ("Patches of precipitation"), ("Partial precipitation"), ("Unknown thunderstorm"),     ("Blowing precipitation"), ("Showers, type unknown"),    ("Drifting precipitation"), ("Freezing precipitation") },
/* MIST          */ {("Mist"),                  ("Mist in the vicinity") ,         ("Light mist"),          ("Moderate mist"),          ("Thick mist"),          ("Shallow mist"),          ("Patches of mist"),          ("Partial mist"),          "??",                           ("Mist with wind"),        "??",                           ("Drifting mist"),          ("Freezing mist")          },
/* FOG           */ {("Fog"),                   ("Fog in the vicinity") ,          ("Light fog"),           ("Moderate fog"),           ("Thick fog"),           ("Shallow fog"),           ("Patches of fog"),           ("Partial fog"),           "??",                           ("Fog with wind"),         "??",                           ("Drifting fog"),           ("Freezing fog")           },
/* SMOKE         */ {("Smoke"),                 ("Smoke in the vicinity") ,        ("Thin smoke"),          ("Moderate smoke"),         ("Thick smoke"),         ("Shallow smoke"),         ("Patches of smoke"),         ("Partial smoke"),         ("Smoke w/ thunders"),        ("Smoke with wind"),       "??",                           ("Drifting smoke"),         "??"                         },
/* VOLCANIC_ASH  */ {("Volcanic ash"),          ("Volcanic ash in the vicinity") , "??",                      ("Moderate volcanic ash"),  ("Thick volcanic ash"),  ("Shallow volcanic ash"),  ("Patches of volcanic ash"),  ("Partial volcanic ash"),  ("Volcanic ash w/ thunders"), ("Blowing volcanic ash"),  ("Showers of volcanic ash "), ("Drifting volcanic ash"),  ("Freezing volcanic ash")  },
/* SAND          */ {("Sand"),                  ("Sand in the vicinity") ,         ("Light sand"),          ("Moderate sand"),          ("Heavy sand"),          "??",                        ("Patches of sand"),          ("Partial sand"),          "??",                           ("Blowing sand"),          "None",                             ("Drifting sand"),          "??"                         },
/* HAZE          */ {("Haze"),                  ("Haze in the vicinity") ,         ("Light haze"),          ("Moderate haze"),          ("Thick haze"),          ("Shallow haze"),          ("Patches of haze"),          ("Partial haze"),          "??",                           ("Haze with wind"),        "??",                           ("Drifting haze"),          ("Freezing haze")          },
/* SPRAY         */ {("Sprays"),                ("Sprays in the vicinity") ,       ("Light sprays"),        ("Moderate sprays"),        ("Heavy sprays"),        ("Shallow sprays"),        ("Patches of sprays"),        ("Partial sprays"),        "??",                           ("Blowing sprays"),        "??",                           ("Drifting sprays"),        ("Freezing sprays")        },
/* DUST          */ {("Dust"),                  ("Dust in the vicinity") ,         ("Light dust"),          ("Moderate dust"),          ("Heavy dust"),          "??",                        ("Patches of dust"),          ("Partial dust"),          "??",                           ("Blowing dust"),          "??",                           ("Drifting dust"),          "??"                         },
/* SQUALL        */ {("Squall"),                ("Squall in the vicinity") ,       ("Light squall"),        ("Moderate squall"),        ("Heavy squall"),        "??",                        "??",                           ("Partial squall"),        ("Thunderous squall"),        ("Blowing squall"),        "??",                           ("Drifting squall"),        ("Freezing squall")        },
/* SANDSTORM     */ {("Sandstorm"),             ("Sandstorm in the vicinity") ,    ("Light standstorm"),    ("Moderate sandstorm"),     ("Heavy sandstorm"),     ("Shallow sandstorm"),     "??",                           ("Partual sandstorm"),     ("Thunderous sandstorm"),     ("Blowing sandstorm"),     "??",                           ("Drifting sandstorm"),     ("Freezing sandstorm")     },
/* DUSTSTORM     */ {("Duststorm"),             ("Duststorm in the vicinity") ,    ("Light duststorm"),     ("Moderate duststorm"),     ("Heavy duststorm"),     ("Shallow duststorm"),     "??",                           ("Partial duststorm"),     ("Thunderous duststorm"),     ("Blowing duststorm"),     "??",                           ("Drifting duststorm"),     ("Freezing duststorm")     },
/* FUNNEL_CLOUD  */ {("Funnel cloud"),          ("Funnel cloud in the vicinity") , ("Light funnel cloud"),  ("Moderate funnel cloud"),  ("Thick funnel cloud"),  ("Shallow funnel cloud"),  ("Patches of funnel clouds"), ("Partial funnel clouds"), "??",                           ("Funnel cloud w/ wind"),  "??",                           ("Drifting funnel cloud"),  "??"                         },
/* TORNADO       */ {("Tornado"),               ("Tornado in the vicinity") ,      "??",                      ("Moderate tornado"),       ("Raging tornado"),      "??",                        "??",                           ("Partial tornado"),       ("Thunderous tornado"),       ("Tornado"),               "??",                           ("Drifting tornado"),       ("Freezing tornado")       },
/* DUST_WHIRLS   */ {("Dust whirls"),           ("Dust whirls in the vicinity") ,  ("Light dust whirls"),   ("Moderate dust whirls"),   ("Heavy dust whirls"),   ("Shallow dust whirls"),   ("Patches of dust whirls"),   ("Partial dust whirls"),   "??",                           ("Blowing dust whirls"),   "??",                           ("Drifting dust whirls"),   "??"                         }
};

const char *winddir_str[] = {
    ("None"), ("Variable"),
    ("North"), ("North - NorthEast"), ("Northeast"), ("East - NorthEast"),
    ("East"), ("East - Southeast"), ("Southeast"), ("South - Southeast"),
    ("South"), ("South - Southwest"), ("Southwest"), ("West - Southwest"),
    ("West"), ("West - Northwest"), ("Northwest"), ("North - Northwest")
};

enum {
	RES_BLANK = 0,
	RES_UPDATE = 2,
	RES_SUN = 4,
	RES_PART = 6,
	RES_CLOUD = 8,
	RES_RAIN = 10,
	RES_SNOW = 12,
	RES_LIGHT = 14
};

enum {
	TIME_RE = 0,
	WIND_RE,
	VIS_RE,
	CLOUD_RE,
	TEMP_RE,
	PRES_RE,
	COND_RE,

	RE_NUM
};

struct WeatherData {
	WeatherData();
	~WeatherData();

	char	time_str[256];
	int		temp;
	int		dew;
	int		sky;
	int		phem;
	int		qual;
	int		winddir;
	int		windspeed;
	double	vis;
	double	pres;
	int		hum;

	regex_t	metar_re[RE_NUM];
};

WeatherData::WeatherData()
{
	time_str[0] = 0;
	temp = VALUE_NONE;
	dew = VALUE_NONE;
	sky = WeatherStatus::SKY_NONE;
	phem = WeatherStatus::PHENOMENON_NONE;
	qual = QUALIFIER_NONE;
	winddir = WIND_NONE;
	windspeed = VALUE_NONE;
	vis = VALUE_NONE;
	pres = VALUE_NONE;
	hum = VALUE_NONE;

	regcomp(&metar_re[TIME_RE], TIME_RE_STR, REG_EXTENDED);
	regcomp(&metar_re[WIND_RE], WIND_RE_STR, REG_EXTENDED);
	regcomp(&metar_re[VIS_RE], VIS_RE_STR, REG_EXTENDED);
	regcomp(&metar_re[CLOUD_RE], CLOUD_RE_STR, REG_EXTENDED);
	regcomp(&metar_re[TEMP_RE], TEMP_RE_STR, REG_EXTENDED);
	regcomp(&metar_re[PRES_RE], PRES_RE_STR, REG_EXTENDED);
	regcomp(&metar_re[COND_RE], COND_RE_STR, REG_EXTENDED);
}

WeatherData::~WeatherData()
{
	regfree(&metar_re[TIME_RE]);
	regfree(&metar_re[WIND_RE]);
	regfree(&metar_re[VIS_RE]);
	regfree(&metar_re[CLOUD_RE]);
	regfree(&metar_re[TEMP_RE]);
	regfree(&metar_re[PRES_RE]);
	regfree(&metar_re[COND_RE]);
}

void parse_metar(WeatherStatus *weather, const char* location, char* string);

// #pragma mark - WeatherStatus

// constructor
WeatherStatus::WeatherStatus(const char* location)
	: fLocation(location)

	, fSkyCondition(SKY_NONE)
	, fPhenomenon(PHENOMENON_NONE)
	, fTemperature(0.0)

	, fLastUpdated(LONGLONG_MIN)
{
}

// WeatherStatus
WeatherStatus::~WeatherStatus()
{
}

// Location
const char*
WeatherStatus::Location() const
{
	return fLocation.String();
}

// SkyCondition
WeatherStatus::sky_condition
WeatherStatus::SkyCondition() const
{
	return fSkyCondition;
}

// Phenomenon
WeatherStatus::phenomenon
WeatherStatus::Phenomenon() const
{
	return fPhenomenon;
}

// TemperatureCelsius
float
WeatherStatus::TemperatureCelsius() const
{
	return fTemperature;
}

// TemperatureFahrenheit
float
WeatherStatus::TemperatureFahrenheit() const
{
	return fTemperature * 9 / 5 + 32;
}

// ParseMetar
void
WeatherStatus::ParseMetar(char* data)
{
	parse_metar(this, Location(), data);
}

// SetTo
void
WeatherStatus::SetTo(const WeatherData& data)
{
	fTemperature = data.temp;
	fSkyCondition = (sky_condition)data.sky;
	fPhenomenon = (phenomenon)data.phem;
	fLastUpdated = system_time();
}

// #pragma mark - WeatherStatusManager

// default instance
WeatherStatusManager
WeatherStatusManager::sDefaultInstance;

// constructor
WeatherStatusManager::WeatherStatusManager()
	: fLocationStatusMap()
	, fFtpThread(-1)
	, fRunningSem(create_sem(0, "ftp thread running sem"))
{
	if (fRunningSem >= 0) {
		fFtpThread = spawn_thread(_FtpThreadEntry, "ftp weather updater",
			B_LOW_PRIORITY, this);
		if (fFtpThread >= 0) {
			resume_thread(fFtpThread);
		} else {
			printf("WeatherStatusManager::WeatherStatusManager() - "
				"unable to spawn ftp thread: %s\n", strerror(fFtpThread));
		}
	}
}

// destructor
WeatherStatusManager::~WeatherStatusManager()
{
	// cancel FTP thread
	delete_sem(fRunningSem);
	if (fFtpThread >= 0) {
		int32 exitValue = 0;
		wait_for_thread(fFtpThread, &exitValue);
	}

	// delete WeatherStatus objects
	HashMap<HashString, WeatherStatus*>::Iterator iterator
		= fLocationStatusMap.GetIterator();
	while (iterator.HasNext())
		delete iterator.Next().value;
}

// Default
/*static*/ WeatherStatusManager*
WeatherStatusManager::Default()
{
	return &sDefaultInstance;
}

// FetchInformationFor
WeatherStatus*
WeatherStatusManager::FetchInformationFor(const char* location)
{
	if (LockWithTimeout(1000) < B_OK)
		return NULL;

	WeatherStatus* status = NULL;
	if (fLocationStatusMap.ContainsKey(location))
		status = fLocationStatusMap.Get(location);

	if (!status) {
		status = new (nothrow) WeatherStatus(location);
		if (!status) {
			printf("WeatherStatusManager::FetchInformationFor() - "
				"no memory to create WeatherStatus\n");
			Unlock();
			return NULL;
		}
		if (fLocationStatusMap.Put(location, status) < B_OK) {
			printf("WeatherStatusManager::FetchInformationFor() - "
				"no memory to add WeatherStatus to hash map\n");
			Unlock();
			delete status;
			return NULL;
		}
		// a new status has been added, cause the FTP thread
		// to wakeup immediately
		release_sem(fRunningSem);
	}

	Unlock();

	if (fFtpThread < 0)
		_DownloadStatus(status);

	return status;
}

// #pragma mark -

// _FtpThreadEntry
/*static*/ int32
WeatherStatusManager::_FtpThreadEntry(void* cookie)
{
	WeatherStatusManager* manager = (WeatherStatusManager*)cookie;
	return manager->_FtpThread();
}

// _FtpThread
int32
WeatherStatusManager::_FtpThread()
{
	while (true) {
		// wait a bit
		// NOTE: this will return B_OK right when a new WeatherStatus
		// has been added to the maintained list
		status_t ret = acquire_sem_etc(fRunningSem, 1, B_RELATIVE_TIMEOUT,
			60 * 1000000);
		if (ret != B_OK && ret != B_TIMED_OUT && ret != B_INTERRUPTED)
			break;

		// iterate over all known WeatherStatuses
		// and see if it is time to uptdate them
		BAutolock _(this);
		LocationStatusMap::Iterator iterator = fLocationStatusMap.GetIterator();
		while (iterator.HasNext()) {
			// this does not really download anything if
			// the time interval between updates has not passed
			_DownloadStatus(iterator.Next().value);
		}
	}
	return 0;
}

// _DownloadStatus
void
WeatherStatusManager::_DownloadStatus(WeatherStatus* status)
{
	BAutolock _(status);
	
	int32 updateInterval = 15 * 60;

	if (status->LastUpdated() + updateInterval * 1000000 > system_time())
		return;

	const char* location = status->Location();

	// construct path to output file and see if it exists and is recent enough
	BString output("/tmp/");
	output << location;

	BEntry outputEntry(output.String());
	time_t entryTime = 0;
	time_t now = time(NULL);
	if (!outputEntry.Exists() || !outputEntry.IsFile()
		|| outputEntry.GetModificationTime(&entryTime) < B_OK
		|| now - entryTime > updateInterval) {
		// download data
		BString command("wget ");
		command << "-q "; // quiet
		command << "-t 5 "; // five retries
		command << "-T 10 "; // timeout after 10 seconds
		command << "-w 1 "; // one second between retries
		command << "-O " << output << " "; // output file
		command << "ftp://tgftp.nws.noaa.gov/data/observations/metar/stations/";
		command << location << ".TXT"; // the location file we want to download
	
		if (system(command.String()) != 0) {
			printf("WeatherStatusManager::FetchInformationFor() - "
				"failed to download METAR weather status\n");
			outputEntry.SetTo(output.String());
			if (outputEntry.Exists())
				outputEntry.Remove();
			return;
		}
	}

	// read data and parse it
	BFile input(output.String(), B_READ_ONLY);
	status_t err = input.InitCheck();
	if (err < B_OK) {
		printf("WeatherStatusManager::FetchInformationFor(%s):\n"
			"  failed to open downloaded METAR weather status file %s (%s)\n",
			location, output.String(), strerror(err));
		return;
	}
	off_t bufferSize;
	err = input.GetSize(&bufferSize);
	if (err < B_OK) {
		printf("WeatherStatusManager::FetchInformationFor(%s):\n"
			"  failed to get file size %s\n", location, strerror(err));
		return;
	}

	if (bufferSize > 64 * 1024) {
		printf("WeatherStatusManager::FetchInformationFor(%s):\n"
			"  unreasonable file size %lld\n", location, bufferSize);
		return;
	}

	char* buffer = new (nothrow) char[bufferSize + 1];
	ssize_t read = input.Read(buffer, bufferSize);
	if (read != (ssize_t)bufferSize) {
		printf("WeatherStatusManager::FetchInformationFor(%s):\n"
			"  failed to read file contents %s\n", location,
			strerror((status_t)read));
		delete[] buffer;
		return;
	}
	// make sure buffer is terminated properly
	buffer[bufferSize] = 0;

//printf("%s\n", location);
//fwrite(buffer, bufferSize, 1, stdout);	

	//parse it
	parse_metar(status, location, buffer);
	delete[] buffer;
}

// #pragma mark -

bool
metar_tok_time(WeatherData* weather, char *tok)
{
	char sday[3], shr[3], smin[3];
	int day, hr, min;
	
	if (regexec(&weather->metar_re[TIME_RE], tok, 0, NULL, 0) == REG_NOMATCH)
		return false;
	
	strncpy(sday, tok, 2);
	sday[2] = 0;
	day = atoi(sday);
	
	strncpy(shr, tok+2, 2);
	shr[2] = 0;
	hr = atoi(shr);
	
	strncpy(smin, tok+4, 2);
	smin[2] = 0;
	min = atoi(smin);
	
	time_t now;
	struct tm *tm;
	
	now = time(NULL);
	//tm = localtime(&now);
	tm = localtime(&now);
	
	/* Estimate timezone */
	/*if (gtm->tm_mday == ltm->tm_mday)
		hour_diff = gtm->tm_hour - ltm->tm_hour;
    else
		if ((gtm->tm_mday == ltm->tm_mday + 1) || ((gtm->tm_mday == 1) && (ltm->tm_mday >= 27)))
			hour_diff = gtm->tm_hour + (24 - ltm->tm_hour);
		else
			hour_diff = -((24 - gtm->tm_hour) + ltm->tm_hour);*/
	
	//alert("http: %i\ntime: %i\ngmtoff: %i", hr, tm->tm_hour, tm->tm_gmtoff/3600);
	//alert("%i", timezone);

	tm->tm_mday = day;
	tm->tm_hour = hr + tm->tm_gmtoff/3600;
	if (tm->tm_hour < 0)
		tm->tm_hour += 24;
	tm->tm_min = min;
	
	strftime(weather->time_str, sizeof(weather->time_str), "%l:%M %P"/* %B %e"*/, tm);
	//strftime(weather->time_str, sizeof(weather->time_str), "%l:%M %P %B %e", ltm);
	//sprintf(weather->time_str, "to do");
	if (weather->time_str[0] == ' ') {
		for (uint i=0; i<strlen(weather->time_str); i++)
			weather->time_str[i] = weather->time_str[i+1];
	}
	//strcat(weather->time_str, " (GMT)");
	
	return true;
}

bool
metar_tok_temp(WeatherData* weather, char *tok)
{
	if (regexec(&weather->metar_re[TEMP_RE], tok, 0, NULL, 0) == REG_NOMATCH)
		return false;
	
	char *temp, *dew;
	temp = tok;
	dew = strchr(tok, '/');
	*dew = 0;
	dew++;
	
	if (*temp == 'M')
		weather->temp = -atoi(temp+1);
	else
		weather->temp = atoi(temp);
	
	if (*dew == 'M')
		weather->dew = -atoi(dew+1);
	else
		weather->dew = atoi(dew);
	
	return true;
}

bool
metar_tok_sky(WeatherData* weather, char *tok)
{
	char stype[4], salt[4];
	int alt = -1;
	
	if (regexec(&weather->metar_re[CLOUD_RE], tok, 0, NULL, 0) == REG_NOMATCH)
		return false;
	
	strncpy(stype, tok, 3);
	stype[3] = 0;
	if (strlen(tok) == 6) {
		strncpy(salt, tok+3, 3);
		salt[3] = 0;
		alt = atoi(salt);  /* Altitude - currently unused */
	}
	
	if (!strcmp(stype, "CLR"))
		weather->sky = WeatherStatus::SKY_CLEAR;
	else if (!strcmp(stype, "BKN"))
		weather->sky = WeatherStatus::SKY_BROKEN;
	else if (!strcmp(stype, "SCT"))
		weather->sky = WeatherStatus::SKY_SCATTERED;
	else if (!strcmp(stype, "FEW"))
		weather->sky = WeatherStatus::SKY_FEW;
	else if (!strcmp(stype, "OVC"))
		weather->sky = WeatherStatus::SKY_OVERCAST;
	
	return true;
}

bool
metar_tok_cond(WeatherData* weather, char *tok)
{
	char squal[3], sphen[4];
	char *pphen;
	
	if (regexec(&weather->metar_re[COND_RE], tok, 0, NULL, 0) == REG_NOMATCH)
		return false;
	
	if ((strlen(tok) > 3) && ((*tok == '+') || (*tok == '-')))
		++tok;   /* FIX */
	
	if ((*tok == '+') || (*tok == '-'))
		pphen = tok + 1;
	else if (strlen(tok) < 4)
		pphen = tok;
	else
		pphen = tok + 2;
	
	memset(squal, 0, sizeof(squal));
	strncpy(squal, tok, pphen - tok);
	squal[pphen - tok] = 0;
	
	memset(sphen, 0, sizeof(sphen));
	strcpy(sphen, pphen);
	
	/* Defaults */
	weather->qual = QUALIFIER_NONE;
	weather->phem = WeatherStatus::PHENOMENON_NONE;
	
	if (!strcmp(squal, ""))
		weather->qual = QUALIFIER_MODERATE;
	else if (!strcmp(squal, "-"))
		weather->qual = QUALIFIER_LIGHT;
	else if (!strcmp(squal, "+"))
		weather->qual = QUALIFIER_HEAVY;
	else if (!strcmp(squal, "VC"))
		weather->qual = QUALIFIER_VICINITY;
	else if (!strcmp(squal, "MI"))
		weather->qual = QUALIFIER_SHALLOW;
	else if (!strcmp(squal, "BC"))
		weather->qual = QUALIFIER_PATCHES;
	else if (!strcmp(squal, "PR"))
		weather->qual = QUALIFIER_PARTIAL;
	else if (!strcmp(squal, "TS"))
		weather->qual = QUALIFIER_THUNDERSTORM;
	else if (!strcmp(squal, "BL"))
		weather->qual = QUALIFIER_BLOWING;
	else if (!strcmp(squal, "SH"))
		weather->qual = QUALIFIER_SHOWERS;
	else if (!strcmp(squal, "DR"))
		weather->qual = QUALIFIER_DRIFTING;
	else if (!strcmp(squal, "FZ"))
		weather->qual = QUALIFIER_FREEZING;
	
	if (!strcmp(sphen, "DZ"))
		weather->phem = WeatherStatus::PHENOMENON_DRIZZLE;
	else if (!strcmp(sphen, "RA"))
		weather->phem = WeatherStatus::PHENOMENON_RAIN;
	else if (!strcmp(sphen, "SN"))
		weather->phem = WeatherStatus::PHENOMENON_SNOW;
	else if (!strcmp(sphen, "SG"))
		weather->phem = WeatherStatus::PHENOMENON_SNOW_GRAINS;
	else if (!strcmp(sphen, "IC"))
		weather->phem = WeatherStatus::PHENOMENON_ICE_CRYSTALS;
	else if (!strcmp(sphen, "PE"))
		weather->phem = WeatherStatus::PHENOMENON_ICE_PELLETS;
	else if (!strcmp(sphen, "GR"))
		weather->phem = WeatherStatus::PHENOMENON_HAIL;
	else if (!strcmp(sphen, "GS"))
		weather->phem = WeatherStatus::PHENOMENON_SMALL_HAIL;
	else if (!strcmp(sphen, "UP"))
		weather->phem = WeatherStatus::PHENOMENON_UNKNOWN_PRECIPITATION;
	else if (!strcmp(sphen, "BR"))
		weather->phem = WeatherStatus::PHENOMENON_MIST;
	else if (!strcmp(sphen, "FG"))
		weather->phem = WeatherStatus::PHENOMENON_FOG;
	else if (!strcmp(sphen, "FU"))
		weather->phem = WeatherStatus::PHENOMENON_SMOKE;
	else if (!strcmp(sphen, "VA"))
		weather->phem = WeatherStatus::PHENOMENON_VOLCANIC_ASH;
	else if (!strcmp(sphen, "SA"))
		weather->phem = WeatherStatus::PHENOMENON_SAND;
	else if (!strcmp(sphen, "HZ"))
		weather->phem = WeatherStatus::PHENOMENON_HAZE;
	else if (!strcmp(sphen, "PY"))
		weather->phem = WeatherStatus::PHENOMENON_SPRAY;
	else if (!strcmp(sphen, "DU"))
		weather->phem = WeatherStatus::PHENOMENON_DUST;
	else if (!strcmp(sphen, "SQ"))
		weather->phem = WeatherStatus::PHENOMENON_SQUALL;
	else if (!strcmp(sphen, "SS"))
		weather->phem = WeatherStatus::PHENOMENON_SANDSTORM;
	else if (!strcmp(sphen, "DS"))
		weather->phem = WeatherStatus::PHENOMENON_DUSTSTORM;
	else if (!strcmp(sphen, "PO"))
		weather->phem = WeatherStatus::PHENOMENON_DUST_WHIRLS;
	else if (!strcmp(sphen, "+FC"))
		weather->phem = WeatherStatus::PHENOMENON_TORNADO;
	else if (!strcmp(sphen, "FC"))
		weather->phem = WeatherStatus::PHENOMENON_FUNNEL_CLOUD;
	
	return true;
}

bool
metar_tok_wind(WeatherData* weather, char *tok)
{
	char sdir[4], sspd[4], sgust[4];
	int dir, spd, gust = -1;
	char *gustp;
	
	if (regexec(&weather->metar_re[WIND_RE], tok, 0, NULL, 0) == REG_NOMATCH)
		return false;
	
	strncpy(sdir, tok, 3);
	sdir[3] = 0;
	dir = (!strcmp(sdir, "VRB")) ? -1 : atoi(sdir);
	
	memset(sspd, 0, sizeof(sspd));
	strncpy(sspd, tok+3, strspn(tok+3, CONST_DIGITS));
	spd = atoi(sspd);
	
	gustp = strchr(tok, 'G');
	if (gustp) {
		memset(sgust, 0, sizeof(sgust));
		strncpy(sgust, gustp+1, strspn(gustp+1, CONST_DIGITS));
		gust = atoi(sgust);
	}
	
	if ((349 <= dir) && (dir <= 11))
		weather->winddir = WIND_N;
	else if ((12 <= dir) && (dir <= 33))
		weather->winddir = WIND_NNE;
	else if ((34 <= dir) && (dir <= 56))
		weather->winddir = WIND_NE;
	else if ((57 <= dir) && (dir <= 78))
		weather->winddir = WIND_ENE;
	else if ((79 <= dir) && (dir <= 101))
		weather->winddir = WIND_E;
	else if ((102 <= dir) && (dir <= 123))
		weather->winddir = WIND_ESE;
	else if ((124 <= dir) && (dir <= 146))
		weather->winddir = WIND_SE;
	else if ((147 <= dir) && (dir <= 168))
		weather->winddir = WIND_SSE;
	else if ((169 <= dir) && (dir <= 191))
		weather->winddir = WIND_S;
	else if ((192 <= dir) && (dir <= 213))
		weather->winddir = WIND_SSW;
	else if ((214 <= dir) && (dir <= 236))
		weather->winddir = WIND_SW;
	else if ((247 <= dir) && (dir <= 258))
		weather->winddir = WIND_WSW;
	else if ((259 <= dir) && (dir <= 281))
		weather->winddir = WIND_W;
	else if ((282 <= dir) && (dir <= 303))
		weather->winddir = WIND_WNW;
	else if ((304 <= dir) && (dir <= 326))
		weather->winddir = WIND_NW;
	else if ((327 <= dir) && (dir <= 348))
		weather->winddir = WIND_NNW;
	
	weather->windspeed = spd;
	
	return true;
}

bool
metar_tok_vis(WeatherData* weather, char *tok)
{
	char *pfrac, *pend;
	char sval[4];
	int val;
	
	if (regexec(&weather->metar_re[VIS_RE], tok, 0, NULL, 0) == REG_NOMATCH)
		return false;
	
	pfrac = strchr(tok, '/');
	pend = strstr(tok, "SM");
	memset(sval, 0, sizeof(sval));
	
	if (pfrac) {
		strncpy(sval, pfrac + 1, pend - pfrac - 1);
		val = atoi(sval);
		weather->vis = (*tok == 'M') ? 0.001 : (1.0 / val);
	} else {
		strncpy(sval, tok, pend - tok);
		val = atoi(sval);
		weather->vis = val;
	}
	
	return true;
}

bool
metar_tok_pres(WeatherData* weather, char *tok)
{
	if (regexec(&weather->metar_re[PRES_RE], tok, 0, NULL, 0) == REG_NOMATCH)
		return false;
	
	if (*tok == 'A') {
		char sintg[3], sfract[3];
		int intg, fract;
		
		strncpy(sintg, tok + 1, 2);
		sintg[2] = 0;
		intg = atoi(sintg);
		
		strncpy(sfract, tok + 3, 2);
		sfract[2] = 0;
		fract = atoi(sfract);
		
		weather->pres = intg + (fract / 100.0);
	} else {  /* *tokp == 'Q' */
		char spres[5];
		int pres;
		
		strncpy(spres, tok+1, 4);
		spres[4] = 0;
		pres = atoi(spres);
		
		weather->pres = pres * 0.02963742;
	}
	
	return true;
}

void
parse_metar(WeatherStatus *weather, const char* code, char* string)
{
	char token[32];
	sprintf(token, "\n%s", code);
	string = strstr(string, token);
	if (!string) {
		printf("parse_metar() - data is for different location "
			"than that of WeatherStatus\n");
		return;
	}

	string += 6;
	//look for double return which ends the metar section
	char *end = strstr(string, "\n\n");
	if (end)
		*end = 0;
	
	if (!weather->Lock())
		return;

	WeatherData data;
	
	char *tok = strtok(string, " \n");
	while (tok) {
		if (metar_tok_time(&data, tok))
			;
		else if (metar_tok_temp(&data, tok))
			;
		else if (metar_tok_sky(&data, tok))
			;
		else if (metar_tok_cond(&data, tok))
			;
		else if (metar_tok_wind(&data, tok))
			;
		else if (metar_tok_vis(&data, tok))
			;
		else if (metar_tok_pres(&data, tok))
			;
//		else
//			printf("unknown metar token \"%s\"\n", tok);
		tok = strtok(0, " \n");
	}
	
	// calc humidity
	double esat, esurf;
	esat = 6.11 * pow(10.0, (7.5 * data.temp) / (237.7 + data.temp));
	esurf = 6.11 * pow(10.0, (7.5 * data.dew) / (237.7 + data.dew));
	data.hum = (int)((esurf / esat) * 100.0);

//	// translate to english system if god wills it
//	if (!metric) {
//		if (weather->temp != VALUE_NONE)
//			weather->temp = weather->temp * 9 / 5 + 32;
//		if (weather->temp != VALUE_NONE)
//			weather->dew = weather->dew * 9 / 5 + 32;
//	} else {
//		if (weather->windspeed != VALUE_NONE)
//			weather->windspeed = (int)(weather->windspeed * 1.851965);
//		if (weather->vis != VALUE_NONE)
//			weather->vis = weather->vis * 1.609344;
//		if (weather->pres != VALUE_NONE)
//			weather->pres = weather->pres * 25.4;
//	}
	
	weather->SetTo(data);

	weather->Unlock();
}

/*
void parse_iwin(WeatherData* weather, char *zone, char *string)
{
	char *p, *rangep = NULL;
	regmatch_t match[1];
	int ret;
	
	p = string;
	while ((ret = regexec(&weather->iwin_re, p, 1, match, 0)) != REG_NOMATCH)
	{
		rangep = p + match[0].rm_so;
		
		p = strchr(rangep, '\n');
		if (iwin_range_match(rangep, zone))
			break;
	}
	
	if (ret != REG_NOMATCH)
	{
		char *end = strstr(p, "\n</PRE>");
		if ((regexec(&weather->iwin_re, p, 1, match, 0) != REG_NOMATCH) && (end - p > match[0].rm_so))
			end = p + match[0].rm_so - 1;
		*end = 0;
		
		weather->fore = new char[strlen(rangep) + 1];
		strcpy(weather->fore, rangep);
	}
	else
	{
		//char *error = "Forecast not available";
		char *error = "Forecast not available in this area.\n\nForecasts are only available in some parts of the united states.";
		weather->fore = new char[strlen(error) + 1];
		strcpy(weather->fore, error);
	}
	
	//from here on out it's all cosmetic to make it not look like human defication

	//end it proporly
	char *end = strstr(weather->fore, "\r\n\r\n$$");
	if (end)
		*end = 0;
	end = strstr(weather->fore, "\r\n\r\n=\r\n$$");
	if (end)
		*end = 0;

//translate removes from and adds to from weather->fore
#define TRANSLATE(from, to) \
{\
	char *bad = strstr(weather->fore, from);\
	while (bad)\
	{\
		strncpy(bad, to, strlen(to));\
		bad += strlen(to);\
		for (uint i=0; i<strlen(bad) + strlen(to) - strlen(from) + 1; i++)\
			bad[i] = bad[i + strlen(from) - strlen(to)];\
		bad = strstr(bad, from);\
	}\
}
	TRANSLATE("\r\n\r\n", "\n\n");
	TRANSLATE("\r\n", " ");
	TRANSLATE("\n...", "\n");
	TRANSLATE("\n.", "\n");
	TRANSLATE("...", ": ");
	TRANSLATE(" .", "\n");
	
	TRANSLATE(" -", "-");
	TRANSLATE("- ", "-");
	TRANSLATE("  ", " ");
#undef TRANSLATE
	
	//there are these anoying tables of temperatures that some times get reported
	//they are ugly, and I don't want to have to deal with them, so we strip them out.
	//they start with < and last an entire line. find and kill that line.
	end = strchr(weather->fore, '<');
	if (end)
	{
		char *start = end;
		end = strchr(start, '\n');
		int skip = end - start + 1;
		for (int i=0; i<int(strlen(start)) - skip + 1; i++)
			start[i] = start[i + skip];
	}
	
	//make it lowercase in preperation for upper-casing certain things
	for (uint i=0; i<strlen(weather->fore); i++)
		weather->fore[i] = tolower(weather->fore[i]);

//translate now makes the character AFTER start uppercase
#define TRANSLATE(start) \
{\
	char *bad = strstr(weather->fore, start);\
	while (bad)\
	{\
		*(bad + strlen(start)) = toupper(*(bad + strlen(start)));\
		bad += strlen(start);\
		bad = strstr(bad, start);\
	}\
}
	weather->fore[0] = toupper(weather->fore[0]);
		//first char should be caped too
	TRANSLATE("\n");
	TRANSLATE("-");
	TRANSLATE(": ");
	TRANSLATE(". ");
#undef TRANSLATE
}

bool iwin_range_match(char *range, char *zone)
{
	char *zonep;
	char *endp;
	char zone_state[4], zone_num_str[4];
	int zone_num;
	
	endp = range + strcspn(range, " \t\r\n") - 1;
	if (strspn(endp - 6, CONST_DIGITS) == 6)
	{
		--endp;
		while (*endp != '-')
			--endp;
	}
	
	strncpy(zone_state, zone, 3);
	zone_state[3] = 0;
	strncpy(zone_num_str, zone + 3, 3);
	zone_num_str[3] = 0;
	zone_num = atoi(zone_num_str);
	
	zonep = range;
	while (zonep < endp)
	{
		char from_str[4], to_str[4];
		int from, to;
		
		if (strncmp(zonep, zone_state, 3) != 0)
		{
			zonep += 3;
			zonep += strcspn(zonep, CONST_ALPHABET "\n");
			continue;
		}
		
		zonep += 3;
		
		do
		{
			strncpy(from_str, zonep, 3);
			from_str[3] = 0;
			from = atoi(from_str);
			
			zonep += 3;
			
			if (*zonep == '-')
			{
				++zonep;
				to = from;
			}
			else if (*zonep == '>')
			{
				++zonep;
				strncpy(to_str, zonep, 3);
				to_str[3] = 0;
				to = atoi(to_str);
				zonep += 4;
			}
			else
			{
				to = 0;
			}
		
		if ((from <= zone_num) && (zone_num <= to))
			return true;
		
		}while (!isupper(*zonep) && (zonep < endp));
	}
	
	return false;
}
*/
