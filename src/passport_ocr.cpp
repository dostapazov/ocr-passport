#include <future>
#include <stdio.h>
#include <math.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <leptonica/allheaders.h>
#include <iostream>
#include <spawn.h>
#include <sys/wait.h>
#include <passport-ocr.h>



namespace  passport_ocr {
typedef std::vector<std::future<bool>>  future_vect;

template <typename T>
void wait_all_done(std::vector<std::future<T>> &fvect)
{
    for(auto & x : fvect)
        x.wait();
}



inline double grad2rad(double angle)
{
  return angle*(M_PI/180.0);
}

__attribute__((visibility("hidden")))
PIX*  rotate_pix(PIX * pixs,double angle,bool free_src_pix) noexcept
{
  PIX * pixr = nullptr;
  switch(abs(int(angle*100.0)))
  {
    case     0: pixr = pixs;free_src_pix = false;break;
    case  9000: pixr = pixRotate90 (pixs,angle <0 ? -1:1);break;
    case 27000: pixr = pixRotate90 (pixs,angle <0 ? 1:-1);break;
    case 18000: pixr = pixRotate180(nullptr,pixs);break;
    default   : pixr = pixRotate   (pixs,grad2rad(angle),L_ROTATE_SAMPLING,L_BRING_IN_BLACK,std::max(pixs->w,pixs->h),std::max(pixs->w,pixs->h));
      break;
  }
  if(free_src_pix) pixDestroy(&pixs);
  return pixr;
}

PIX*  background_norm(PIX * pixs,bool free_src_pix) noexcept
{
    PIX * pix;
    //pix = pixBackgroundNorm(pixs,NULL,NULL,5,5,80,16,190,1,1);
    pix = pixBackgroundNormSimple(pixs,NULL,NULL);
    if(free_src_pix)   pixDestroy(&pixs);
   return pix;
}

PIX*  unsharp_masking(PIX * pixs,int half_width,double fract, bool free_src) noexcept
{
    PIX * pix;
    //pix = pixBackgroundNorm(pixs,NULL,NULL,5,5,80,16,190,1,1);
    pix = pixUnsharpMasking(pixs,half_width,fract);
    if(!pix ) {pix = pixs; free_src = false;}
    if(free_src)   pixDestroy(&pixs);
   return pix;
}


__attribute__((visibility("hidden")))

bool do_save_image(PIX * pix,const std::string & name)
{

  if(pix)
  {
      boost::system::error_code ec;
      boost::filesystem::remove(name.c_str(),ec);
      return pixWrite(name.c_str(),pix,IFF_PNG) ? false : true;
  }
  return false;
}

__attribute__((visibility("hidden")))
PIX * create_dest_pix(const PIX * src_pix,const ocr_area_t & ar)
{
  if(!src_pix || 0==ar.rects_count()) return nullptr;

    double xfactor = double(src_pix->xres)/300.0;
    double yfactor = double(src_pix->yres)/300.0;
    int destW = 0,destH = 0;
    bool is_vertical = ar.rects()[0].is_vertical();

    for(auto & rct : ar.rects() )
    {

     if(is_vertical != rct.is_vertical()) {return nullptr; /*throw ocr_exception("mix vertical and horizontal ocr rects");*/}
     if(is_vertical)
     { destH = std::max(destH,rct.h());destW+= rct.w(); }
     else
     {destW = std::max(destW,rct.w()); destH+= rct.h();}
    }

    destW = ceil(double(destW)*xfactor);
    destH = ceil(double(destH)*yfactor);
    PIX * pix = pixCreate(destW,destH,src_pix->d);
    pix->xres = src_pix->xres;
    pix->yres = src_pix->yres;
    int destX = 0,destY = 0;
    for(auto  rct : ar.rects() )
    {
        rct.rescale(xfactor,yfactor);
        //pixRasterop(pix,destX,destY,rct.w(),rct.h(),PIX_SRC,src_pix,rct.x(),rct.y)
        pixRasterop(pix,destX,destY,rct.w(),rct.h(),PIX_SRC,const_cast<PIX*>(src_pix),rct.x(),rct.y());
        if(is_vertical)
            destX += rct.w();
        else
           destY += rct.h();

    }
    return pix;
}

__attribute__((visibility("hidden")))
bool  do_make_image(const PIX * src_pix,const std::string & res_folder,const ocr_area_t & ar)
{
   bool ret = false;
   if(src_pix)
   {
       PIX * pix = create_dest_pix(src_pix,ar);
    if(pix)
    {
     if(ar.rects()[0].is_vertical())

     pix = rotate_pix(pix,ar.angle(),true);

     if(int(ar.contrast()))  pix = pixContrastTRC(NULL,pix,1.0+ar.contrast());
      pix = background_norm(pix,true);
      pix = unsharp_masking(pix,3,0.3,true);

     ret  = do_save_image  (pix,ar.image_name(res_folder));
     pixDestroy(&pix);
    }
   }
   return ret;
}

__attribute__((visibility("hidden")))
void make_crop_images(const std::string  &src_file, const std::string &res_folder, ocr_areas_t & area)
{

  PIX * pix    = pixRead(src_file.c_str());
  if(!pix) return ;
  PIX * igs = NULL;// pixConvertRGBToGrayFast()
  future_vect fv;
  for(auto & ar : area)
  {

     if(ar.is_text() && !igs) igs = pixConvertRGBToGrayFast(pix);
     fv.push_back(std::async(do_make_image,ar.is_text() ? igs : pix,res_folder,ar));

  }
  wait_all_done(fv);
  pixDestroy(&igs);
  pixDestroy(&pix);
}


bool run_tesseract(const std::string & folder, const ocr_area_t & r)
{
  std::string cmd ;
  cmd+="tesseract ";
  cmd+= r.image_name(folder);
  cmd+= " " ;
  cmd+= r.file_name(folder,"");
  cmd+= " -l " ;
  cmd+= r.get_lang();
  int res = system(cmd.c_str());
  return res ?  false : true;

}

void passport_recognizer_c::do_recognize()
{
  future_vect fv;
  for(auto & ar : m_ocr_areas)
  {
    if(ar.is_text())
      fv.push_back(std::async(run_tesseract,m_result_folder,ar));
  }
  wait_all_done(fv);
}


void passport_recognizer_c::prepare_recognize  ()
{
  make_crop_images(m_src_file,m_result_folder,m_ocr_areas);
}

void passport_recognizer_c::recognize(const std::string & _src_file)
{
   m_src_file = _src_file;
   prepare_recognize();
   do_recognize();
}

void passport_recognizer_c::set_result_folder(const std::string & _result_folder)
{
  m_result_folder = _result_folder;
  boost::trim(m_result_folder);
  if(!m_result_folder.empty()) boost::filesystem::create_directory(m_result_folder);
}

void passport_recognizer_c::fetch_result(results_t & res)
{
  res.clear();
  for(auto & ar : this->m_ocr_areas)
  {
    if(ar.is_text())
    {
      std::ifstream  is(ar.text_name(this->m_result_folder));
      if(is.is_open())
      {
          std::string content( (std::istreambuf_iterator<char>(is) ),
                                 (std::istreambuf_iterator<char>()    ) );

          res[ar.name()] = content;
      }
      else
          res[ar.name()] = ar.image_name(m_result_folder);
    }
  }
}


}
