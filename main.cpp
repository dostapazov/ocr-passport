#include <iostream>
#include "passport-ocr.h"
#include <boost/program_options.hpp>

using namespace std;

void print_usage()
{
   cout<<"ocr-passport source-image [result-folder ] [ocr-area-desript]"<<endl
       <<"          result-dolder     by default = result" <<endl
       <<"          ocr-area-descrpit by default = rus_passport.json"<<endl;
}

int main(int argc, char * argv[])
{

    using namespace passport_ocr ;

    if(argc>1)
   {
        passport_recognizer_c pocr;

        ocr_areas_t areas;
        try
        {
            pocr.read_areas(std::string( argc>3 ? argv[3] : "rus_passport1.json"));
            pocr.set_result_folder(std::string(argc>2 ? argv[2] : "results"));
            pocr.recognize (std::string(argv[1])) ;
            passport_recognizer_c::results_t ocr_result;
            pocr.fetch_result(ocr_result);
            for(auto  p : ocr_result)
                cout<<p.first<<" : "<<endl<<p.second<<endl;

        }
        catch(std::exception & ex)
        {
         cout<<"Program error"<<endl<<ex.what()<<endl;
        }
   }
   else
   print_usage();

    return 0;
}
