#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <fstream>
#include <passport-ocr.h>


namespace passport_ocr {
    namespace ptree = boost::property_tree ;


const char * var_x        = "x";
const char * var_y        = "y";
const char * var_w        = "w";
const char * var_h        = "h";
const char * var_rect_cnt = "rect_count";
const char * var_rect     = "rect";
const char * var_angle    = "angle";
const char * var_istxt    = "is_text";
const char * var_lang     = "lang";
const char * var_contrast = "contrast";

template <typename T>
struct ocr_rect2strings
{

  std::string operator()(const T & src)
  {
      using namespace boost;
      using namespace std;
      return
      lexical_cast<string>(src.x()) + ','+
      lexical_cast<string>(src.y()) + ','+
      lexical_cast<string>(src.w()) + ','+
      lexical_cast<string>(src.h()) ;

  }

};

void  passport_recognizer_c::write_ocr_areas (const std::string & fname,const ocr_areas_t & data )
{
  ptree::ptree pt;
  for(auto  ptr : data)
  {
    std::string  name = ptr.name();
    name+='.';
    pt.add(name+var_istxt   ,boost::lexical_cast<std::string>(ptr.is_text()));
    if(ptr.is_text())
       pt.add(name+var_lang ,ptr.get_lang());
    pt.add(name+var_angle   ,boost::lexical_cast<std::string>(ptr.angle()));
    pt.add(name+var_contrast,boost::lexical_cast<std::string>(ptr.contrast()));
    pt.add(name+var_rect_cnt,boost::lexical_cast<std::string>(ptr.rects().size()));

    int i = 0;
    for(auto & x : ptr.rects())
        pt.add(name+std::string(var_rect) + boost::lexical_cast<std::string>(++i),ocr_rect2strings<Iocr_rect>()(x));



  }
    std::fstream fs(fname);
    ptree::json_parser::write_json(fname,pt);
}


int   passport_recognizer_c::read_ocr_areas(const std::string & fname,      ocr_areas_t & data )
{
   data.clear();
   ptree::ptree pt;
   ptree::json_parser::read_json(fname,pt);
   data.reserve(pt.size());

   for(auto & v : pt)
   {
    auto & sv = v.second;
    ocr_areas_t::value_type ocr(v.first
                                ,sv.get<bool>  (var_istxt)
                                ,sv.get<double>(var_angle)
                                ,sv.get<double>(var_contrast)
                                );

    int rcnt = sv.get<int>(var_rect_cnt);
    for(int i = 0;i<rcnt;)
    {
     std::string rv = sv.get<std::string>(std::string(var_rect)+boost::lexical_cast<std::string>(++i));
     std::vector<std::string> rect_values;
     boost::algorithm::split(rect_values,rv,boost::algorithm::is_any_of(","));
     if(rect_values.size()>3)
        {
         ocr.add_rect(
                       boost::lexical_cast<int>(rect_values[0])
                      ,boost::lexical_cast<int>(rect_values[1])
                      ,boost::lexical_cast<int>(rect_values[2])
                      ,boost::lexical_cast<int>(rect_values[3])
                      );
        }

    }
    if(ocr.is_text())
    {
      try{
         ocr.set_lang(sv.get<std::string>(var_lang));
         }
        catch(...){}
    }
    data.push_back(ocr);
   }


  return data.size();
}

}
