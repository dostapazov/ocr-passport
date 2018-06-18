#ifndef PASSPORTOCR_H
#define PASSPORTOCR_H

#include <string>
#include <vector>
#include <map>

namespace passport_ocr  {

class ocr_exception:public std::exception
{
  public:
    ocr_exception(const char * msg)  throw():message(msg){}
    virtual const char* what() const throw() override final {return message.c_str();}
  private:
        std::string message;
};


template < typename T>
class ocr_rect_t
{

  public:
  ocr_rect_t()
      :m_x(0),m_y(0),m_w(0),m_h(0)
      {}
  ocr_rect_t(T _w,T _h)
      :m_x(0),m_y(0),m_w(_w),m_h(_h)
      {}
  ocr_rect_t(T _x,T _y,T _w,T _h)
      :m_x(_x),m_y(_y),m_w(_w),m_h(_h)
      {}
  template <typename sT>
  ocr_rect_t(const  ocr_rect_t<sT> & src)
     {*this = src; }

  template <typename sT>
  ocr_rect_t & operator =  (const  ocr_rect_t<sT> & src)
  {
    m_x = src.m_x; m_y = src.m_y; m_w = src.m_w; m_h = src.m_h;
    return * this;
  }

  ocr_rect_t & grow(T dx,T dy)
  { m_w+=dx;m_h+=dy; return *this;}
  ocr_rect_t & move(T dx,T dy)
  { m_x+=dx;m_y+=dy; return *this;}

  ocr_rect_t & rescale(double scale_x,double scale_y)
  {
      m_x = T(double(m_x)*scale_x);
      m_y = T(double(m_y)*scale_y);
      m_w = T(double(m_w)*scale_x);
      m_h = T(double(m_h)*scale_y);
      return * this;
  }

  T    x()const {return m_x;}
  T    y()const {return m_y;}
  T    w()const {return m_w;}
  T    h()const {return m_h;}
  T   rx()const {return m_x + m_w;}
  T   by()const {return m_y + m_h;}

  bool is_empty   ()const {return m_w<T(1) || m_h <T(1) ? true : false;}
  bool is_vertical()const {return m_w<m_h ? true : false;}

  ocr_rect_t rect_union (const ocr_rect_t &r )
  {
    // make union rects
    T x0 = std::min(m_x,r.x());
    T y0 = std::min(m_y,r.y());
    T x1 = std::max(rx(),r.rx());
    T y1 = std::max(by(),r.by());
    return ocr_rect_t(x0,y0,x1-x0,y1-y0);
  }

  ocr_rect_t rect_intersect (const ocr_rect_t &r )
  {
     //make intersect rect;
      T x0 = std::max(m_x,r.x());
      T y0 = std::max(m_y,r.y());
      T x1 = std::max(T(0),std::min(rx(),r.rx()));
      T y1 = std::max(T(0),std::min(by(),r.by()));
      return ocr_rect_t(x0,y0,x1-x0,y1-y0);

  }

private:
  T m_x,m_y,m_w,m_h;

};

typedef ocr_rect_t<int>    Iocr_rect;
typedef ocr_rect_t<double> Docr_rect;

class ocr_area_t
{

  public:
    typedef std::vector<Iocr_rect> vrect_t;
  ocr_area_t()
          :m_is_text(false),m_angle(0),m_contrast(0)
  {}
  ocr_area_t(const std::string & _name,int _x,int _y,int _w,int _h,bool _is_text,double _rotate_angle,double _contrast)
          :m_name(_name),m_is_text(_is_text),m_angle(_rotate_angle),m_contrast(_contrast)
  {
    add_rect(_x,_y,_w,_h);
  }

  ocr_area_t(const std::string & _name,bool _is_text,double _rotate_angle,double _contrast)
          :m_name(_name),m_is_text(_is_text),m_angle(_rotate_angle),m_contrast(_contrast)
  {}

  ocr_area_t(const ocr_area_t & src)
  {(*this) = src;}

  ocr_area_t & operator = (const ocr_area_t & src)
  {
    m_name     = src.m_name;
    m_is_text  = src.m_is_text; m_angle = src.m_angle;
    m_contrast = src.m_contrast;
    m_rects    = src.m_rects;
    m_lang     = src.m_lang;
    return * this;
  }

  void    add_rect(int _x,int _y,int _w,int _h)
  {m_rects.push_back(Iocr_rect(_x,_y,_w,_h));}
  void set_lang(const std::string &lng){m_lang = lng;}

 const std::string name () const {return m_name;}
        double     angle() const {return m_angle;}
        double  contrast() const {return m_contrast;}
        bool     is_text() const {return m_is_text;}
 const vrect_t &   rects() const {return m_rects;}
       int   rects_count() const {return m_rects.size();}

 const std::string get_lang  () const {return m_lang.empty() ? std::string("rus") : m_lang;}
 const std::string file_name (const std::string & folder ,const std::string & ext) const ;
 const std::string image_name(const std::string & folder ) const ;
 const std::string text_name (const std::string & folder ) const ;


 private:
 std::string  m_name;
 std::string  m_lang;
 bool    m_is_text;
 double  m_angle;
 double  m_contrast;
 vrect_t m_rects;

};

inline const std::string ocr_area_t::file_name   (const std::string & folder,const std::string & ext ) const
{
  std::string res = folder;
  if(!res.empty())  res+='/';
  res+=m_name;
  res+=ext;
  return res;
}

inline const std::string ocr_area_t::image_name (const std::string & folder ) const
{
  return file_name(folder,".png");
}

inline const std::string ocr_area_t::text_name  (const std::string & folder ) const
{
  return file_name(folder,".txt");
}


typedef std::vector<passport_ocr::ocr_area_t> ocr_areas_t;

class passport_recognizer_c
{
  public:
  typedef std::map<std::string,std::string>  results_t;
   void prepare_recognize  ();
   void do_recognize       ();

   public:
   passport_recognizer_c(){}
       void recognize  (const std::string & _src_file) ;
       void read_areas (const std::string & fname);
       void write_areas(const std::string & fname);
       void set_result_folder(const std::string & _result_folder);
       void fetch_result(results_t & res);

static void  write_ocr_areas (const std::string & fname,const ocr_areas_t & data );
static int   read_ocr_areas  (const std::string & fname,      ocr_areas_t & data );


private:

    ocr_areas_t              m_ocr_areas;
    std::string              m_src_file;
    std::string              m_result_folder;
};


inline void passport_recognizer_c::read_areas(const std::string & fname)
{
  read_ocr_areas(fname,m_ocr_areas);
}

inline void passport_recognizer_c::write_areas(const std::string & fname)
{
    write_ocr_areas(fname,m_ocr_areas);
}

}



#endif // PASSPORTOCR_H
