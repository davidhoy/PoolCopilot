#include "tinybind.h"


// do this to support your STL container types, like list and vector
#if 0
typedef std::vector<int> StlIntVector;
TiXmlBinding<StlIntVector> const *GetTiXmlBinding( StlIntVector const &,  StlIntVector const &  )
{
  static StlContainerTiXmlBinding<int, StlIntVector> binding(false);
  return &binding;
}
#endif


#ifdef WIN32
#undef TIXML_USE_STL
#else
#define TIXML_USE_STL
#endif

#ifdef TIXML_USE_STL
template<class T>
char const *ConvertToString( T const & t )
{
  std::stringstream str;
  static std::string strOut;
  str << t;
  strOut = str.str();
  return strOut.c_str();
}

template<class T>
void ConvertFromString( char const * strIn, T * dataOut )
{
  std::stringstream str;
  str << strIn;
  str >> *dataOut;
}

char const *ConvertToString( XmlDate const & s )
{
    static char buffer[20];
    sprintf( buffer, "%04d/%02d/%02d",
             s.Year, s.Month, s.Day );
    return buffer;
}

char const *ConvertToString( XmlTime const & s )
{
    static char buffer[20];
    sprintf( buffer, "%02d:%02d:%02d",
             s.Hour, s.Minute, s.Second );
    return buffer;
}


template<>
void ConvertFromString<char const *>( char const * strIn, const char * * dataOut )
{
  static std::string strHolder;
  strHolder = strIn;
  *dataOut = strHolder.c_str();
}

#else


char const *ConvertToString( double const & d )
{
  static char buffer[2048];
  sprintf(buffer, "%g", d);
  return buffer;
}

char const *ConvertToString( float const & f )
{
  return ConvertToString((double)f);
}

char const *ConvertToString( int const & d )
{
  static char buffer[2048];
  sprintf(buffer, "%d", d);
  return buffer;
}

char const *ConvertToString( unsigned int const & d )
{
  static char buffer[2048];
  sprintf(buffer, "%u", d);
  return buffer;
}

char const *ConvertToString( char const * const & s )
{
  return s;
}

char const *ConvertToString( std::string const & s )
{
  return s.c_str();
}


#ifdef WIN32
void ConvertFromString<char const *>( char const * strIn, const char * * dataOut )
{
  *dataOut = strIn;
}
#endif

void ConvertFromString( char const * strIn, std::string * dataOut )
{
  *dataOut = strIn;
}

void ConvertFromString( char const * strIn,  int * dataOut )
{
  *dataOut = atoi(strIn);
}

void ConvertFromString( char const * strIn,  unsigned int * dataOut )
{
  *dataOut = (unsigned int) atoi(strIn);
}

void ConvertFromString( char const * strIn,  double * dataOut )
{
  *dataOut = atof(strIn);
}

void ConvertFromString( char const * strIn,  float * dataOut )
{
  *dataOut = (float)atof(strIn);
}
#endif


void ConvertFromString( char const * strIn,  bool * dataOut )
{
    if (stricmp( strIn, "1" ) == 0 )
        *dataOut = true;
    else
        *dataOut = false;
}

#if 0
void ConvertFromString( char const * strIn,  YesNo * dataOut )
{
    if (stricmp( strIn, "yes" ) == 0 )
        *dataOut = Yes;
    else
        *dataOut = No;
}
#endif

#if 0
void ConvertFromString( char const * strIn,  OnOff * dataOut )
{
    if (stricmp( strIn, "on" ) == 0 )
        *dataOut = On;
    else
        *dataOut = Off;
}
#endif

void ConvertFromString( char const * strIn,  XmlDate * dataOut )
{
    sscanf( strIn, "%04d/%02d/%02d",
            &dataOut->Year,
            &dataOut->Month,
            &dataOut->Day );
}

void ConvertFromString( char const * strIn,  XmlTime * dataOut )
{
    sscanf( strIn, "%02d:%02d:%02d",
            &dataOut->Hour,
            &dataOut->Minute,
            &dataOut->Second );
}


template<class T>
TiXmlBinding<T> const *GetTiXmlBinding( T const &, IdentityBase  )
{
  static GenericTiXmlBinding<T> binding;
  return &binding;
}

TiXmlBinding<float> const *GetTiXmlBinding( float const &, IdentityBase  )
{
  static GenericTiXmlBinding<float> binding;
  return &binding;
}

TiXmlBinding<double> const *GetTiXmlBinding( double const &, IdentityBase  )
{
  static GenericTiXmlBinding<double> binding;
  return &binding;
}

TiXmlBinding<int> const *GetTiXmlBinding( int const &, IdentityBase  )
{
  static GenericTiXmlBinding<int> binding;
  return &binding;
}

TiXmlBinding<char const *> const *GetTiXmlBinding( char const * const &, IdentityBase  )
{
  static GenericTiXmlBinding<char const *> binding;
  return &binding;
}

TiXmlBinding<std::string> const *GetTiXmlBinding( std::string const &, IdentityBase  )
{
  static GenericTiXmlBinding<std::string> binding;
  return &binding;
}

TiXmlBinding<bool> const *GetTiXmlBinding( bool const &, IdentityBase  )
{
  static GenericTiXmlBinding<bool> binding;
  return &binding;
}

//TiXmlBinding<YesNo> const *GetTiXmlBinding( YesNo const &, IdentityBase  )
//{
//  static GenericTiXmlBinding<YesNo> binding;
//  return &binding;
//}

//TiXmlBinding<OnOff> const *GetTiXmlBinding( OnOff const &, IdentityBase  )
//{
//  static GenericTiXmlBinding<OnOff> binding;
//  return &binding;
//}

TiXmlBinding<XmlDate> const *GetTiXmlBinding( XmlDate const &, IdentityBase  )
{
  static GenericTiXmlBinding<XmlDate> binding;
  return &binding;
}

TiXmlBinding<XmlTime> const *GetTiXmlBinding( XmlTime const &, IdentityBase  )
{
  static GenericTiXmlBinding<XmlTime> binding;
  return &binding;
}

#if 0
TiXmlBinding<OnOff> const *GetTiXmlBinding( OnOff const &, IdentityBase  )
{
  static GenericTiXmlBinding<OnOff> binding;
  return &binding;
}

TiXmlBinding<Date> const *GetTiXmlBinding( Date const &, IdentityBase  )
{
  static GenericTiXmlBinding<Date> binding;
  return &binding;
}

TiXmlBinding<Time> const *GetTiXmlBinding( Time const &, IdentityBase  )
{
  static GenericTiXmlBinding<Time> binding;
  return &binding;
}
#endif
