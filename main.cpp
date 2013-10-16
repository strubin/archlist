/*
Create by Sergey Trubin at 2013-09-23
*/

//#define _GNU_SOURCE
#include <cstddef>
//#define _GLIBCXX_GTHREAD_USE_WEAK


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <libgen.h>
#include <fnmatch.h>
#include <stdint.h>

#include <unistd.h>
#include <limits.h>
#include <getopt.h>

#include <iostream>
#include <fstream>
#include <magic.h>
#include <sys/stat.h>
//#include <map>
#include <set>
#include <string>
#include <vector>

#include <algorithm>

#define __ << " : " <<
#define LOG(t) std::cout << t << endl;
using namespace std;


string		symlink_mime("inode/symlink; charset=binary");
string		dir_mime    ("inode/directory; charset=binary");
bool		follow_symlink=false;
bool		slink_ignore=false;
bool            use_mime=false;
bool            all_data=false;
bool		empty_dir=true;
bool            only_info=false;
string 		relative_dir;


float    abt2_size = 0.05;
unsigned abt1_size = 32;

string		trim(string src);
string		normalize_path(const string src);
//string		exlude_file;
vector<string>	exflist;
vector<string>	exmlist;
int 		dir(magic_t m,string pathname);
const char*	ext(const string&);
const char*     bname(const string&);
int 		set_list(const char* file,vector<string>& list);
void 		usage(const char*);


struct file_type
{
    string path;
    string mime;
    size_t size;
    __time_t mtime;
//     file_type(const string& f,const string& m,size_t s):path(f),mime(m),size(s) {}
//     file_type(const file_type& ft):path(ft.path),mime(ft.mime),size(ft.size) {}
    file_type(const string& f,const string& m,__time_t t,size_t s):path(f),mime(m),mtime(t),size(s) {}
    file_type(const file_type& ft):path(ft.path),mime(ft.mime),mtime(ft.mtime),size(ft.size) {}
};

struct file_ino
{
    ino_t st_ino;
    dev_t st_dev;
    file_ino(ino_t i,dev_t d):st_ino(i),st_dev(d) {}
    file_ino(const file_ino& f):st_ino(f.st_ino),st_dev(f.st_dev) {}
};


int	mime_compare		(const file_type& s1, const file_type& s2);


int	full_patch_compare	(const file_type& s1, const file_type& s2);
int	mtime_compare		(const file_type& s1, const file_type& s2);
int	size_compare		(const file_type& s1, const file_type& s2);
int	size_abt1_compare	(const file_type& s1, const file_type& s2);
int	size_abt2_compare	(const file_type& s1, const file_type& s2);
int	bname_case_compare	(const file_type& s1, const file_type& s2);
int	bname_case_rcompare	(const file_type& s1, const file_type& s2);
int	bname_compare		(const file_type& s1, const file_type& s2);
int	ext_case_compare	(const file_type& s1, const file_type& s2);
int	ext_case_rcompare	(const file_type& s1, const file_type& s2);
int	ext_compare		(const file_type& s1, const file_type& s2);

typedef int	(*compare_type) (const file_type& s1, const file_type& s2) ;
vector<compare_type> compare_list;

#define RES(t) {return t<0;}

struct ltft
{
    bool operator()(const file_type& s1, const file_type& s2) const
    {
        int res;
        for(auto i:compare_list)
            if(res=i(s1,s2))return res<0;
        return 0;
    }
};


struct ift
{
    bool operator()(const file_ino& s1, const file_ino& s2) const
    {
        if(s1.st_dev!=s2.st_dev)return s1.st_dev < s2.st_dev;
        else return s1.st_ino < s2.st_ino;
    }
};


set<file_type,ltft> main_list;
set<file_ino ,ift > ino_dir_list;
set<file_ino ,ift > ino_link_list;


int main(int argc, char *argv[])
{

    if(argc>1)
    {
        {
            const char* short_options = "hdilLr::x:m:s:a";

            const struct option long_options[] = {
                {"help",no_argument,NULL,'h'},
                {"follow",no_argument,NULL,'l'},
                {"--exlink",no_argument,NULL,'L'},		
                {"relative",optional_argument,NULL,'r'},
                {"sort",no_argument,NULL,'s'},
                {"exfile",required_argument,NULL,'x'},
                {"exmime",required_argument,NULL,'m'},
                {"all",no_argument,NULL,'a'},
                {"dir",no_argument,NULL,'d'},
		{"info",no_argument,NULL,'i'},
                {NULL,0,NULL,0}
            };

            int rez;
            int option_index=0;

            while ((rez=getopt_long(argc,argv,short_options,long_options,&option_index))!=-1)
            {

                switch(rez) {
                case 'h': {
                    usage(argv[0]);
                    //cout << "This is demo help. Try -h or --help." << endl;
                    //cout << option_index << endl;//__ long_options[option_index].name __ long_options[option_index].has_arg __ long_options[option_index].val << endl;
                    return 0;

                };

                case 'l': {
                    follow_symlink=true;
		    slink_ignore=false;
                    break;
                };
                case 'L': {
                    follow_symlink=false;
		    slink_ignore=true;
                    break;
                };		
                case 'r': {


                    if (optarg!=NULL)
                        relative_dir=optarg;
                    else
                    {
                        char pwd[PATH_MAX];
                        if (getcwd(pwd, sizeof(pwd)) == NULL) return EFAULT;
                        relative_dir=string(pwd)+'/';
                    }
                    break;
                };

                case 'x':
                {
                    if(optarg)
                        set_list(optarg,exflist);
                    break;
                };

                case 'm':
                {
                    if(optarg)
                        set_list(optarg,exmlist);
                    use_mime=true;
                    break;
                };
                case 's':
                {
                    if(optarg)
                    {
                        const char* end=optarg+strlen(optarg);
                        for(char* c=optarg; c!=end; c++)
                            switch(*c)
                            {

                            case 'f':
                                compare_list.push_back(full_patch_compare);
                                break;
                            case 'm':
                                compare_list.push_back(mime_compare);
                                use_mime=true;
                                break;
                            case 't':
                                compare_list.push_back(mtime_compare);
                                break;
                            case 's':
                                compare_list.push_back(size_compare);
                                break;
                            case 'S':
                                compare_list.push_back(size_abt1_compare);
                                while(*(c+1)=='S') {
                                    abt1_size<<=1;
                                    ++c;
                                }
                                
                                if(abt1_size==32&&isdigit(*(c+1)))
                                {
				    string abt1_str;
				    while(isdigit(*(c+1))) 
					  abt1_str.push_back(*(++c));
				    abt1_size=atoi(abt1_str.c_str());
				}
                                break;
                            case 'P':
                                compare_list.push_back(size_abt2_compare);
                                while(*(c+1)=='P') {
                                    abt2_size+=0.05;
                                    ++c;
                                }
                                LOG(abt2_size);                                
                                if(abt2_size==float(0.05)&&isdigit(*(c+1))||*(c+1)=='.')
                                {
					  string abt2_str;
					  while(isdigit(*(c+1))||*(c+1)=='.') 
					  	abt2_str.push_back(*(++c));
						
					  abt2_size=atof(abt2_str.c_str())/float(100);
					  
					  if(abt2_size>1)
					  {
						cerr << "approximate size more 100%" << endl;
						return -1;
					  }

				    
				}                                
                                LOG(abt2_size);
                                break;
                            case 'R':
                                compare_list.push_back(bname_case_rcompare);
                                break;
                            case 'b':
                                compare_list.push_back(bname_compare);
                                break;
                            case 'B':
                                compare_list.push_back(bname_case_compare);
                                break;
                            case 'X':
                                compare_list.push_back(ext_case_rcompare);
                                break;
                            case 'e':
                                compare_list.push_back(ext_compare);
                                break;
                            case 'E':
                                compare_list.push_back(ext_case_compare);
                                break;
                            default:
                                cerr << "Method '" << c << "' no exist"<<endl;
                                return -1;

                            };
                    }

                    break;
                };
                case 'a':
                    all_data=true;
                    break;
                case 'd':
                    empty_dir=false;
                    break;
                case 'i':
                    only_info=true;
                    break;		    
                case '?':
                default: {
                    usage(argv[0]);
                    return -1;
                };
                };

            };


        }

        for(auto c:exmlist)
            if(!fnmatch(c.c_str() ,symlink_mime.c_str(), FNM_NOESCAPE))
            {
                slink_ignore=true;
                break;
            }

        if(compare_list.empty())
        {
            compare_list.push_back(mime_compare);
            compare_list.push_back(bname_case_rcompare);
            compare_list.push_back(full_patch_compare);
            use_mime=true;
        }
        else if(compare_list.back()!=full_patch_compare)
            compare_list.push_back(full_patch_compare);

        magic_t M;

	if(only_info)
	{

#define BOOL(t) ((t)?"yes":"no")
	      cout <<    "approximate size in percent	"	<< abt2_size << '%'
		   <<  "\napproximate size		" 	<< abt1_size 
		   <<  "\nfollow symlinks			"	<< BOOL(follow_symlink)
		   <<  "\nsymlinks ignore			" 	<< BOOL(slink_ignore)		   
		   <<  "\nuse mime			" 	<< BOOL(use_mime)
		   <<  "\nall data			"	<< BOOL(all_data)
		   <<  "\ninclude directory		"	<< BOOL(empty_dir)
		   <<  "\nrelative directory		"	<< ((relative_dir.empty())?"not set":relative_dir) << endl;
	      return 0;
	}
	      
        if(use_mime)
        {
            M=magic_open(MAGIC_SYMLINK|MAGIC_MIME);
            if(!M)
            {
                cerr << "Cannot open magic-file" << endl;
                return -1;
            }
        }
        else
        {
            symlink_mime.clear();
            dir_mime.clear();
        }


        argc -= optind;
        argv += optind;
        if(use_mime)magic_load(M,NULL);
        while (argc--)
            dir(M,*argv++);
        if(use_mime)magic_close(M);






        if(!relative_dir.empty())
        {

            //for(set<file_type,ltft>::iterator i=main_list.begin(); i!=main_list.end(); ++i)
            for(auto i:main_list)
            {
                //if(i->path.find(relative_dir)==string::npos)
                if(i.path.find(relative_dir)!=0)
                {
                    relative_dir.clear();
                    break;
                }
            }
        }

        if(relative_dir.empty())
            for(auto i:main_list)
            {
                if(all_data)
                {   cout << i.mtime << " : ";
                    cout.width(8);
                    cout << i.size __ i.mime  __ i.path << endl;
                }
                else cout << i.path << endl;
            }
        else
        {
            string::size_type rl=relative_dir.size();
            for(auto i:main_list)
                cout << i.path.substr(rl) << endl;
        }

    }
    else usage(argv[0]);
//	cout << main_list.size() __ ino_dir_list.size() __ ino_link_list.size() << endl;
//	cout << sizeof(file_type) << endl;
//	cout << relative_dir << endl;
//      cout << slink_ignore << endl;
//      cout << abt1_size __ abt2_size << endl;
    return 0;
}


void 		usage(const char* file)
{
    cout << "Usage: " << file << " -hla -x [exlude file list] -m [exlude mime list] -r [base dir] -s [sorting method]" << endl;
}

int set_list(const char* file,vector<string>& list)
{
    ifstream efl(file,ios::in);
    if(efl.is_open())
    {
        char tstr[PATH_MAX];
        while(efl.getline(tstr,sizeof(tstr)))
        {
            if(tstr[0]!='#')
                list.push_back(tstr);
        }

        return 0;
    }
    else
    {
        cout << "File " << file << " not found" << endl;
        return ENOENT;
    }
}

int dir(magic_t m,string pathname)
{

    for(auto c:exflist)	  //auto flag=FNM_NOESCAPE|FNM_PATHNAME|FNM_PERIOD;
        if(!fnmatch(c.c_str() ,pathname.c_str(), FNM_NOESCAPE))return 0;


    struct stat st;
    char rp[PATH_MAX + 1];
    lstat(pathname.c_str(),&st);

    if(S_ISREG(st.st_mode))
    {

        if(realpath(pathname.c_str(),rp))
        {
            const char* mime="";
            if(use_mime)mime=magic_file(m,rp);

            if(mime)
            {
                for(auto c:exmlist)
                    if(!fnmatch(c.c_str() ,mime, FNM_NOESCAPE))return 0;
                main_list.insert(file_type(rp,mime,st.st_mtime,st.st_size));
            }
            else
                cerr << "not file :" << rp << endl;
        }
    }
    else if(S_ISLNK(st.st_mode)&&!slink_ignore)
    {

        pathname = normalize_path(pathname);

        file_ino fi(st.st_ino,st.st_dev);
        set<file_ino ,ift >::iterator ii=ino_link_list.find(file_ino(fi));

        if(ii==ino_link_list.end())
        {
            ino_link_list.insert(fi);
            main_list.insert(file_type(pathname,symlink_mime,st.st_mtime,st.st_size));

            if(follow_symlink)
            {
                errno=0;

                size_t sf=readlink(pathname.c_str(),rp,sizeof(rp));
                if(sf>0)
                {
                    rp[sf]='\0';
                    if(rp[0]!='/')
                    {

                        char* pname=strdup(pathname.c_str());
                        char* dname=dirname(pname);
                        char rpp[PATH_MAX + 1];
                        strcpy(rpp,dname);
                        free(pname);
                        strcat(rpp,"/");
                        strcat(rpp,rp);
                        pathname=normalize_path(rpp);
                        dir(m,pathname);
                    }
                    else dir(m,rp);

                }
            }
        }
    }
    else if(S_ISDIR(st.st_mode))
    {

        file_ino fi(st.st_ino,st.st_dev);
        set<file_ino ,ift >::iterator ii=ino_dir_list.find(file_ino(fi));
        if(ii==ino_dir_list.end())
        {
            ino_dir_list.insert(fi);
            if(empty_dir)
	    {
		  char rpp[PATH_MAX + 1];
		  if(realpath(pathname.c_str(),rpp))
		  {
		   size_t len=strlen(rpp);	
		   rpp[len]='/';
		   rpp[len+1]='\0';
		   //LOG("dir : " << rpp __  relative_dir.compare(rpp))
		   if(relative_dir.compare(rpp))
		   main_list.insert(file_type(rpp,dir_mime,st.st_mtime,st.st_size));
		  }
	    }
            DIR *dp;
            struct dirent *dent;

            if( (dp = opendir(pathname.c_str())) == NULL) {
                return 1;
            }

            while(dent = readdir(dp))
            {
                if(strcmp(".", dent->d_name) && strcmp("..", dent->d_name))
                {

                    string fullname=pathname +'/' + dent->d_name;
                    dir(m,fullname);

                }
            }
            closedir(dp);
        }
    }
    return 0;
}

string trim(string src)
{
    // trim src
    string::size_type pos = src.find_last_not_of(" \t\n\v\e\b");

    if(pos != string::npos)
    {

        src.erase(pos + 1);

        pos = src.find_first_not_of(" \t\n\v\e\b");

        if(pos != string::npos) src.erase(0, pos);
    }
    else src.erase(src.begin(), src.end());
    return src;
}

string  normalize_path(string src)
{
    src=trim(src);

    if(src.empty())
    {
        errno=EFAULT;
        return string();
    }

    string res;

    if ( src[0] != '/')
    {

        // relative path

        char pwd[PATH_MAX];
        if (getcwd(pwd, sizeof(pwd)) == NULL) {
            errno=EFAULT;
            return string();
        }

        src=string(pwd)+'/'+src;
    }

    else if(src[0]='~' && src[1]=='/')
    {
        src=normalize_path(getenv("HOME")+'/' + src.substr(2));
    }



    {
        string::size_type pos=1;
        string::size_type next=0;
        do
        {
            pos=next+1;
            next=src.find('/',pos);
            if(pos==next)continue;

            string d=src.substr(pos,next-pos);

            if(d==".")continue;

            if(d=="..")
            {
                string::size_type back=res.rfind('/');
                res.erase(back);
                continue;
            }

            res+='/'+d;

        }
        while(std::string::npos != next);
    }
    string::size_type last=res.size()-1;
    if(res[last]=='/')res.erase(last);

    return res;
}
/*
const char* ext(const string& fname)
{
    for(const char* i=fname.c_str()+fname.size()-1; i!=fname.c_str(); i--)
        if(*i=='.')return i+1;
        else if(*i=='/')return NULL;
    return NULL;
}

const char* bname(const string& fname)
{
    for(const char* i=fname.c_str()+fname.size()-1; i!=fname.c_str(); i--)
        if(*i=='/')return i+1;
    return NULL;
}
*/

int	_case_rcompare	(const file_type& s1, const file_type& s2,char delim)
{
    const string& p1=  s1.path;
    const string& p2=  s2.path;
    string::const_reverse_iterator i1=p1.rbegin();
    string::const_reverse_iterator i2=p2.rbegin();

    while(i1!=p1.rend()&&i2!=p2.rend())
    {
        int c1=toupper(*i1);
        int c2=toupper(*i2);
        if(c1==c2)
        {
            ++i1;
            ++i2;
        }
        else return c1-c2;
        if(*i1==delim||*i2==delim)
        {
            if(*i1!=delim)return 1;
            else if(*i2!=delim)return -1;
        }
    }
    return 0;
}

int	bname_case_rcompare	(const file_type& s1, const file_type& s2)
{
    return     _case_rcompare(s1,s2,'/');
}

int	ext_case_rcompare	(const file_type& s1, const file_type& s2)
{
    return     _case_rcompare(s1,s2,'.');
}


int	full_patch_compare	(const file_type& s1, const file_type& s2)
{
    return  s1.path.compare(s2.path);
}

int	mime_compare		(const file_type& s1, const file_type& s2)
{
    return  s1.mime.compare(s2.mime);
}


int	mtime_compare		(const file_type& s1, const file_type& s2)
{
    return  s1.mtime - s2.mtime;
}

int	size_compare		(const file_type& s1, const file_type& s2)
{
    return  s1.size - s2.size;
}

int	size_abt1_compare		(const file_type& s1, const file_type& s2)
{
    auto dif=s1.size-s2.size;
    if(abs(dif)<abt1_size)return 0;
    else return dif	      ;
}

int	size_abt2_compare		(const file_type& s1, const file_type& s2)
{
    float dif=((float)s1.size+1)/(s2.size+1);
    if(dif>(1.0-abt2_size)&&dif<(1.0+abt2_size))return 0;
    else return s1.size-s2.size	      ;
}


int _compare_def(const string& p1,const string& p2)
{
    return p1.compare(p2);
}

int _compare_case(const string& p1,const string& p2)
{
    return strcasecmp(p1.c_str(),p2.c_str());
}

int	_compare		(const file_type& s1, const file_type& s2,const char* delim,int (*comp)(const string& p1,const string& p2) )
{
    string p1=  s1.path;
    string p2=  s2.path;

    string::size_type r1=p1.find_last_of(delim);
    string::size_type r2=p2.find_last_of(delim);

    if(r1==string::npos||r2==string::npos)
    {
        if(r2==string::npos)return 1;
        else if(r1==string::npos)return -1;
        else return 0;
    }

    p1=p1.substr(++r1);
    p2=p2.substr(++r2);
    return  comp(p1,p2);
}


int	bname_compare		(const file_type& s1, const file_type& s2)
{
    return _compare(s1,s2,"/",_compare_def);
}

int	ext_compare		(const file_type& s1, const file_type& s2)
{
    return _compare(s1,s2,"./",_compare_def);
}


int	bname_case_compare		(const file_type& s1, const file_type& s2)
{
    return _compare(s1,s2,"/",_compare_case);
}

int	ext_case_compare		(const file_type& s1, const file_type& s2)
{
    return _compare(s1,s2,"./",_compare_case);
}
