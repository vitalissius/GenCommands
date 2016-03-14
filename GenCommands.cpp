#include "tinyxml2lib/tinyxml2.h"

#include <iostream>
#include <fstream>
#include <iterator>
#include <streambuf>
#include <string>
#include <vector>
#include <memory>

#include <locale>
#include <codecvt>

#include <algorithm>
#include <random>       // std::default_random_engine
#include <chrono>       // std::chrono::system_clock

#include <io.h>         // _O_U8TEXT
#include <fcntl.h>      // _setmode

std::locale gLocUTF8 = std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>());
std::wstring_convert<std::codecvt_utf8<wchar_t>> gConv;

std::size_t computeFileSize(const std::shared_ptr<std::wifstream> pfs)
{
  pfs->seekg(0, std::ios::end);
  std::size_t size = (std::size_t)pfs->tellg();
  pfs->seekg(0, std::ios::beg);
  return size;
}

struct TvSeries {
  std::wstring _name;
  std::wstring _locname;
  int _year;
  int _amount;
  bool _status;
  std::vector<std::wstring> _vgenres;
  std::vector<std::wstring> _vcountries;

  TvSeries(const std::wstring& name, const std::wstring& locname,
           const int year, const int amount, const bool status,
           const std::vector<std::wstring>& vgenres, const std::vector<std::wstring>& vcountries)
           :
           _name(name),
           _locname(locname),
           _year(year),
           _amount(amount),
           _status(status),
           _vgenres(vgenres),
           _vcountries(vcountries)
  {}

  friend std::wostream& operator<<(std::wostream& o, const TvSeries& tvs);
};

std::wostream& operator<<(std::wostream& o, const TvSeries& tvs)
{
  o << L"Name:       " << tvs._name 
    << L"\nLocname:    " << tvs._locname
    << L"\nYear:       " << std::to_wstring(tvs._year)
    << L"\nAmount:     " << std::to_wstring(tvs._amount)
    << L"\nStatus:     " << std::to_wstring(tvs._status)
    << L"\nGenres:     ";
  for (auto s : tvs._vgenres) o << s << L" ";
  o << L"\nCountries:  ";
  for (auto s : tvs._vcountries) o << s << L" ";
  o << L"\n\n";
  return o;
}

std::shared_ptr<std::vector<std::wstring>> parseThem(const std::string& xmlFile, const std::string& rootElem, const std::string& elems)
{
  std::shared_ptr<std::vector<std::wstring>> pvs = nullptr;

  std::shared_ptr<std::wifstream> pwifs(new std::wifstream(xmlFile), [](std::wifstream* p)
  {
    p->close();
    delete p;
  });

  if (pwifs->is_open())
  {
    pwifs->imbue(gLocUTF8);
    pvs = std::make_shared<std::vector<std::wstring>>();

    std::wstring xmlWstring;
    xmlWstring.reserve(computeFileSize(pwifs));
    std::copy(std::istreambuf_iterator<wchar_t>(*pwifs), std::istreambuf_iterator<wchar_t>(), std::back_inserter(xmlWstring));

    tinyxml2::XMLDocument xmlDoc;
    std::string xmlString = gConv.to_bytes(xmlWstring).data();
    if (xmlDoc.Parse(xmlString.data(), xmlString.length()) != tinyxml2::XML_NO_ERROR)
    {
      throw std::exception((std::string("error parse file: ") += xmlFile).data());
    }

    tinyxml2::XMLElement* country = xmlDoc.FirstChildElement(rootElem.data())->FirstChildElement(elems.data());
    while (country)
    {
      pvs->push_back(gConv.from_bytes(country->GetText()));
      country = country->NextSiblingElement();
    }
  }
  return pvs;
}


std::shared_ptr<std::vector<std::wstring>> parseCountries(const std::string& xmlFile)
{
  return parseThem(xmlFile, "countries", "country");
}

std::shared_ptr<std::vector<std::wstring>> parseGenres(const std::string& xmlFile)
{
  return parseThem(xmlFile, "genres", "genre");
}


std::shared_ptr<std::vector<TvSeries>> parseTvSeries(const std::string xmlFile)
{
  std::shared_ptr<std::vector<TvSeries>> pvtvs = nullptr;

  std::shared_ptr<std::wifstream> pwifs(new std::wifstream(xmlFile), [](std::wifstream* p)
  {
    p->close();
    delete p;
  });

  if (pwifs->is_open())
  {
    pwifs->imbue(gLocUTF8);
    pvtvs = std::make_shared<std::vector<TvSeries>>();

    std::wstring xmlWstring;
    xmlWstring.reserve(computeFileSize(pwifs));
    xmlWstring.assign(std::istreambuf_iterator<wchar_t>(*pwifs), std::istreambuf_iterator<wchar_t>());

    tinyxml2::XMLDocument xmlDoc;
    std::string xmlString = gConv.to_bytes(xmlWstring);
    if (xmlDoc.Parse(xmlString.data(), xmlString.length()) != tinyxml2::XML_NO_ERROR)
    {
      throw std::exception((std::string("error parse file: ") += xmlFile).data());
    }

    auto computeNumberElements = [&xmlDoc]()
    {
      tinyxml2::XMLNode* elem = xmlDoc.FirstChildElement("tvseries")->FirstChildElement("tvs");
      int count = 1;
      while (elem = elem->NextSiblingElement()) ++count;
      return count;
    };
    pvtvs->reserve(computeNumberElements());

    tinyxml2::XMLElement* xeTvseries = xmlDoc.FirstChildElement("tvseries");
    tinyxml2::XMLElement* xeTvs = xeTvseries->FirstChildElement("tvs");
    tinyxml2::XMLElement* xeInfo;
    tinyxml2::XMLElement* xeGenres;
    tinyxml2::XMLElement* xeCountries;

    std::wstring name;                      // Name
    std::wstring locname;                   // Local name
    int year;                               // Release year
    int amount;                             // Seasons amount
    bool status;                            // Filming status
    std::vector<std::wstring> vgenres;      // Genre
    std::vector<std::wstring> vcountries;   // Country

    while (xeTvs)
    {
      static const std::collate<wchar_t>& col = std::use_facet<std::collate<wchar_t>>(gLocUTF8);

      name = gConv.from_bytes(xeTvs->ToElement()->Attribute("name"));

      locname = gConv.from_bytes(xeTvs->ToElement()->Attribute("locname"));

      year = xeTvs->ToElement()->IntAttribute("year");

      xeInfo = xeTvs->FirstChildElement("info");
      amount = xeInfo->ToElement()->IntAttribute("amount");

      static std::wstring c1 = L"снимается";
      std::wstring c2 = gConv.from_bytes(xeInfo->ToElement()->Attribute("status"));
      status = (col.compare(c1.data(), c1.data() + c1.length(), c2.data(), c2.data() + c2.length()) == 0) ? true : false;

      xeGenres = xeInfo->NextSiblingElement("genres")->FirstChildElement("genre");
      while (xeGenres)
      {
        vgenres.push_back(gConv.from_bytes(xeGenres->GetText()));
        xeGenres = xeGenres->NextSiblingElement("genre");
      }

      xeCountries = xeInfo->NextSiblingElement("countries")->FirstChildElement("country");
      while (xeCountries)
      {
        vcountries.push_back(gConv.from_bytes(xeCountries->GetText()));
        xeCountries = xeCountries->NextSiblingElement("country");
      }

      pvtvs->emplace_back(name, locname, year, amount, status, vgenres, vcountries);

      vgenres.clear();
      vcountries.clear();

      xeTvs = xeTvs->NextSiblingElement("tvs");
    }
  }
  return pvtvs;
}

void createQuery_InsertIntoCountriesTable(const std::string& nameSavedFile, const std::shared_ptr<std::vector<std::wstring>> pvs)
{
  if (!pvs || pvs->empty()) throw std::exception("empty parameter in function createQuery_InsertIntoCountriesTable()");

  std::wofstream wout(nameSavedFile);
  if (wout.is_open())
  {
    wout.imbue(gLocUTF8);
    wout << L"INSERT INTO CountriesTb (CountryName) VALUES\n";
    for (auto iter = pvs->begin(); iter != pvs->end(); ++iter)
    {
      wout << L"('" << *iter << L"')";
      if (iter != --pvs->end()) wout << L",\n";
      else wout << L";\n";
    }
    wout.close();
  }
}

void createQuery_InsertIntoGenresTable(const std::string& nameSavedFile, const std::shared_ptr<std::vector<std::wstring>> pvs)
{
  if (!pvs || pvs->empty()) throw std::exception("empty parameter in function createQuery_InsertIntoGenresTable()");

  std::wofstream wout(nameSavedFile);
  if (wout.is_open())
  {
    wout.imbue(gLocUTF8);
    wout << L"INSERT INTO GenresTb (GenreName) VALUES\n";
    for (auto iter = pvs->begin(); iter != pvs->end(); ++iter)
    {
      wout << L"('" << *iter << L"')";
      if (iter != --pvs->end()) wout << L",\n";
      else wout << L";\n";
    }
    wout.close();
  }
}

void createQuery_InsertIntoTvSeriesTable(const std::string& nameSavedFile, const std::shared_ptr<std::vector<TvSeries>> pvtvs)
{
  if (!pvtvs || pvtvs->empty()) throw std::exception("empty parameter in function createQuery_InsertIntoTvSeriesTable()");

  std::wofstream wout(nameSavedFile);
  if (wout.is_open())
  {
    wout.imbue(gLocUTF8);
    wout << L"INSERT INTO TvSeriesTb (SeriesName, ReleaseYear, SeasonsAmount, Status) VALUES \n";
    for (auto iter = pvtvs->begin(); iter != pvtvs->end(); ++iter)
    {
      wout << L"('" << iter->_locname << L"', " << iter->_year << L", " << iter->_amount << L", " << iter->_status << L")";

      if (iter != --pvtvs->end()) 
        wout << L",\n";
      else 
        wout << L";\n";
    }
    wout.close();
  }
}

void createQuery_InsertIntoTvGenreTable(const std::string& nameSavedFile, const std::shared_ptr<std::vector<TvSeries>> pvtvs)
{
  if (!pvtvs || pvtvs->empty()) throw std::exception("empty parameter in function createQuery_InsertIntoTvGenreTable()");

  std::wofstream wout(nameSavedFile);
  if (wout.is_open())
  {
    wout.imbue(gLocUTF8);
    wout << L"INSERT INTO TvGenreTb (SeriesId, GenreId) VALUES \n";
    for (auto itv = pvtvs->begin(); itv != pvtvs->end(); ++itv)
    {
      for (std::vector<std::wstring>::iterator ig = itv->_vgenres.begin(); ig != itv->_vgenres.end(); ++ig)
      {
        wout << L"((SELECT SeriesId FROM TvSeriesTb WHERE SeriesName = '" << itv->_locname << L"'), ";
        wout << L"(SELECT GenreId FROM GenresTb WHERE GenreName = '" << *ig << L"'))";

        if ((itv != --pvtvs->end()) || (ig != --itv->_vgenres.end())) 
          wout << L",\n";
        else 
          wout << L";\n";
      }
    }
    wout.close();
  }
}

void createQuery_InsertIntoTvCountryTable(const std::string& nameSavedFile, const std::shared_ptr<std::vector<TvSeries>> pvtvs)
{
  if (!pvtvs || pvtvs->empty()) throw std::exception("empty parameter in function createQuery_InsertIntoTvCountryTable()");

  std::wofstream wout(nameSavedFile);
  
  if (wout.is_open())
  {
    wout.imbue(gLocUTF8);
    wout << L"INSERT INTO TvCountryTb (SeriesId, CountryId) VALUES \n";
    for (auto itv = pvtvs->begin(); itv != pvtvs->end(); ++itv)
    {
      for (std::vector<std::wstring>::iterator ic = itv->_vcountries.begin(); ic != itv->_vcountries.end(); ++ic)
      {
        wout << L"((SELECT SeriesId FROM TvSeriesTb WHERE SeriesName = '" << itv->_locname << L"'), ";
        wout << L"(SELECT CountryId FROM CountriesTb WHERE CountryName = '" << *ic << L"'))";

        if ((itv != --pvtvs->end()) || (ic != --itv->_vcountries.end())) 
          wout << L",\n";
        else 
          wout << L";\n";
      }
    }
    wout.close();
  }
}

int main()
{
  _setmode(_fileno(stdout), _O_U8TEXT);
  try
  {
    auto pvsCountries = parseCountries("xml\\countries.xml");
    createQuery_InsertIntoCountriesTable("1_insert_into_countries_tb.sql", pvsCountries);

    auto pvsGenres = parseGenres("xml\\genres.xml");
    createQuery_InsertIntoGenresTable("2_insert_into_genres_tb.sql", pvsGenres);

    auto pvsTvSeries = parseTvSeries("xml\\tvseries.xml");
    std::shuffle(pvsTvSeries->begin(), pvsTvSeries->end(), 
                 std::default_random_engine((unsigned int)std::chrono::system_clock::now().time_since_epoch().count()));
    createQuery_InsertIntoTvSeriesTable("3_insert_into_tvseries_tb.sql", pvsTvSeries);
    createQuery_InsertIntoTvCountryTable("4_insert_into_tvcountry_tb.sql", pvsTvSeries);
    createQuery_InsertIntoTvGenreTable("5_insert_into_tvgenre_tb.sql", pvsTvSeries);
  }
  catch (std::exception e)
  {
    _setmode(_fileno(stdout), _O_TEXT);
    std::cout << e.what() << std::endl;
    _setmode(_fileno(stdout), _O_U8TEXT);
  }
  std::wcout << L"Push 'Enter' to exit ...";
  std::wcin.get();
  return 0;
}
