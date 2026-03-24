#include"mpublic.h"

namespace idc{
    //删除str左边的cc,默认是空格
    char* dellstr(char* str,const char cc){
        if(str==nullptr)return nullptr;
        char*p=str;
        while(*p==cc){
            p++;
        }
        memmove(str,p,strlen(str)-strlen(p)+1);
        return str;
    }
    
    string& dellstr(string& str,const char cc){
        auto pos = str.find_first_not_of(cc);
        if(pos!=0)
            str.replace(0,pos,"");
        return str;

    }

    char* delrstr(char* str,const char cc){
        if (str == nullptr) return nullptr;
        if (*str == '\0') return str;  // 空字符串直接返回    
    // 从字符串末尾向前查找
        char* end = str + strlen(str) - 1;   
    // 从右向左找到第一个不是cc的字符
        while (end >= str && *end == cc) {
            end--;
        }
    // 在找到的位置后设置结束符，删除后面所有的cc字符
    if (end >= str) {
        *(end + 1) = '\0';
    }
    else {
        // 整个字符串都是cc字符
        str[0] = '\0';
    } 
    return str;
}

    string& delrstr(string& str,const char cc){
        auto i=str.find_last_not_of(cc);
        if(i!=0)
            str.erase(i+1);
        return str;
    }

    char* dellrstr(char* str,const char cc){
        delrstr(str,cc);
        dellstr(str,cc);
        return str;
    }

    string& dellrstr(string& str,const char cc){
        delrstr(str,cc);
        dellstr(str,cc);
        return str;
    }

    char* toupper(char *str){
        if(str==nullptr)return nullptr;
        char*p=str;
        while(*p!='\0'){
            *p=std::toupper(*p);
            p++;
        }
    return str;
    }

    string& toupper(string &str){
        for(int i =0;i<str.length();i++){
            str[i]=std::toupper(str[i]);
        }
        return str;
    }

    char* tolower(char *str){
        if(str==nullptr)return nullptr;
        char*p=str;
        while(*p!='\0'){
            *p=std::tolower(*p);
            p++;
        }
        return str;
    }

    string& tolower(string &str){
        for(int i=0;i<str.length();i++){
            str[i]=std::tolower(str[i]);
        }
        return str;
    }

    bool replacestr(char *str   ,const string &str1,const string &str2,const bool bloop){
       if (str == nullptr) return false;
        string strtemp(str);
        replacestr(strtemp,str1,str2,bloop);
        strtemp.copy(str,strtemp.length());
        str[strtemp.length()]=0;    // string的copy函数不会给C风格字符串的结尾加0。
        return true;
    }
    bool replacestr(string &str,const string &str1,const string &str2,const bool bloop){
        if ( (str.length() == 0) || (str1.length() == 0) ) return false;
  
    // 如果bloop为true并且str2中包函了str1的内容，直接返回，因为会进入死循环，最终导致内存溢出。
        if ( (bloop==true) && (str2.find(str1)!=string::npos) ) return false;

        int pstart=0;      // 如果bloop==false，下一次执行替换的开始位置。
        int ppos=0;        // 本次需要替换的位置。

        while (true)
            {
            if (bloop == true)
                ppos=str.find(str1);                      // 每次从字符串的最左边开始查找子串str1。
            else
                ppos=str.find(str1,pstart);            // 从上次执行替换的位置后开始查找子串str1。

            if (ppos == string::npos) break;       // 如果没有找到子串str1。

            str.replace(ppos,str1.length(),str2);   // 把str1替换成str2。

            if (bloop == false) pstart=ppos+str2.length();    // 下一次执行替换的开始位置往右移动。
        }
    return true; 
    }


char* picknumber(const string &src,char *dest,const bool bsigned,const bool bdot){
    if(dest==nullptr)return nullptr;
    string p=picknumber(src,bsigned,bdot);
    memset(dest,0,sizeof(dest));
    strcpy(dest,p.c_str());
    return dest;
}


string& picknumber(const string &src,string &dest,const bool bsigned,const bool bdot){
    string p;
    for(int i =0;i<src.length();i++){
        if((src[i]=='+'||src[i]=='-')&&bsigned==true){ 
            p=p.append(1,src[i]);
             continue;
            }
        if(src[i]=='.'&&bdot==true){
            p=p.append(1,'.');
            continue;
        }
        if(isdigit(src[i])){
            p=p.append(1,src[i]);
            continue;
        }
    }
    dest=p;
    return dest;
}


string picknumber(const string &src,const bool bsigned,const bool bdot){
    string res;
    picknumber(src,res,bsigned,bdot);
    return res;
}


bool matchstr(const string &str,const string &rules)
{
    // 如果匹配规则表达式的内容是空的，返回false。
    if (rules.length() == 0) return false;

    // 如果如果匹配规则表达式的内容是"*"，直接返回true。
    if (rules == "*") return true;

    int  ii,jj;
    int  pos1,pos2;
    ccmdstr cmdstr,cmdsubstr;

    string filename=str;
    string matchstr=rules;

    // 把字符串都转换成大写后再来比较
    toupper(filename);
    toupper(matchstr);

    cmdstr.splittocmd(matchstr,",");

    for (ii=0;ii<cmdstr.size();ii++)
    {
        // 如果为空，就一定要跳过，否则就会被匹配上。
        if (cmdstr[ii].empty() == true) continue;

        pos1=pos2=0;
        cmdsubstr.splittocmd(cmdstr[ii],"*");

        for (jj=0;jj<cmdsubstr.size();jj++)
        {
            // 如果是文件名的首部
            if (jj == 0)
                if (filename.substr(0,cmdsubstr[jj].length())!=cmdsubstr[jj]) break;

            // 如果是文件名的尾部
            if (jj == cmdsubstr.size()-1)
                if (filename.find(cmdsubstr[jj],filename.length()-cmdsubstr[jj].length()) == string::npos) break;

            pos2=filename.find(cmdsubstr[jj],pos1);

            if (pos2 == string::npos) break;

            pos1=pos2+cmdsubstr[jj].length();
        }

        if (jj==cmdsubstr.size()) return true;
    }

    return false;
}



// 把字符串拆分到m_cmdstr容器中。
// buffer：待拆分的字符串。
// rules：buffer字符串中字段内容的分隔符，注意，分隔符是字符串，如","、" "、"|"、"~!~"。
// isdelspace：是否删除拆分后的字段内容前后的空格，true-删除；false-不删除，缺省不删除。
void ccmdstr::splittocmd(const string& buffer,const string& sepstr,const bool bdelspace){
    if(buffer.empty()||sepstr.empty()) return;
    m_cmdstr.clear();
    int pos1=0;
    int pos=0;
    string tmp;
    while((pos1=buffer.find(sepstr,pos))!=string::npos){
        tmp =buffer.substr(pos,pos1-pos);
        if(bdelspace==true) dellrstr(tmp);
        m_cmdstr.push_back(tmp);
        pos=pos1+sepstr.length();
    }
    tmp=buffer.substr(pos);
    if(bdelspace==true) dellrstr(tmp);
    m_cmdstr.push_back(tmp);
    return;
}


  // 从m_cmdstr容器获取字段内容。
    // ii：字段的顺序号，类似数组的下标，从0开始。
    // value：传入变量的地址，用于存放字段内容。
    // 返回值：true-成功；如果ii的取值超出了m_cmdstr容器的大小，返回失败。
    //ilen是容器值内的大小
bool ccmdstr::getvalue(const int ii, string & value, const int ilen) const
{
    if(ii>=m_cmdstr.size()||ii<0)  return false;
    if(ilen>0&&ilen<m_cmdstr[ii].length()){
        value=m_cmdstr[ii].substr(0,ilen);
    }
    return true;
}

bool ccmdstr::getvalue(const int ii, char * value, const int len) const
{
    if(ii>=static_cast<int>(m_cmdstr.size())||ii<0)  return false;
    if(len<0||value==nullptr) return false;

    if (len>0) memset(value,0,len+1);   // 调用者必须保证value的空间足够，否则这里会内存溢出。

    if ( (m_cmdstr[ii].length()<=(unsigned int)len) || (len==0) )
    {
        m_cmdstr[ii].copy(value,m_cmdstr[ii].length());
        value[m_cmdstr[ii].length()]=0;    // string的copy函数不会给C风格字符串的结尾加0。
    }
    else
    {
        m_cmdstr[ii].copy(value,len);
        value[len]=0;
    }
    return true;
}


bool ccmdstr::getvalue(const int ii, int & value) const
{  
    if(ii>=m_cmdstr.size()||ii<0)  return false;
    try{
        value=stoi(picknumber(m_cmdstr[ii],false,false));
    }
    catch(const std::exception& e){
        std::cerr << "转换失败: " << e.what() 
              << ", 字符串: " << m_cmdstr[ii] << std::endl;
        return false;
    }
    return true;
}

bool ccmdstr::getvalue(const int ii, unsigned int & value) const
{
if(ii>=m_cmdstr.size()||ii<0)  return false;
    try{
        value=stoul(picknumber(m_cmdstr[ii],false,false));
    }
    catch(const std::exception& e){
        std::cerr << "转换失败: " << e.what() 
              << ", 字符串: " << m_cmdstr[ii] << std::endl;
        return false;
    }
    return true;
}

bool ccmdstr::getvalue(const int ii, long & value) const
{
if(ii>=m_cmdstr.size()||ii<0)  return false;
    try{
        value=stol(picknumber(m_cmdstr[ii],false,false));
    }
    catch(const std::exception& e){
        std::cerr << "转换失败: " << e.what() 
              << ", 字符串: " << m_cmdstr[ii] << std::endl;
        return false;
    }
    return true;return false;
}

bool ccmdstr::getvalue(const int ii, unsigned long & value) const
{
if(ii>=m_cmdstr.size()||ii<0)  return false;
    try{
        value=stoul(picknumber(m_cmdstr[ii],false,false));
    }
    catch(const std::exception& e){
        std::cerr << "转换失败: " << e.what() 
              << ", 字符串: " << m_cmdstr[ii] << std::endl;
        return false;
    }
    return true;return false;
}

bool ccmdstr::getvalue(const int ii, double & value) const
{
if(ii>=m_cmdstr.size()||ii<0)  return false;
    try{
        //cout<<m_cmdstr[ii];
        value=stod(picknumber(m_cmdstr[ii],true,true));
    }
    catch(const std::exception& e){
        std::cerr << "转换失败: " << e.what() 
              << ", 字符串: " << m_cmdstr[ii] << std::endl;
        return false;
    }
    return true;
}

bool ccmdstr::getvalue(const int ii, float & value) const
{
if(ii>=m_cmdstr.size()||ii<0)  return false;
    try{
        value=stof(picknumber(m_cmdstr[ii],false,false));
    }
    catch(const std::exception& e){
        std::cerr << "转换失败: " << e.what() 
              << ", 字符串: " << m_cmdstr[ii] << std::endl;
        return false;
    }
    return true;return false;
}

bool ccmdstr::getvalue(const int ii, bool & value) const
{
    if(ii>=m_cmdstr.size()||ii<0)  return false;
    string tmp=m_cmdstr[ii];
    if(toupper(tmp)=="TRUE"){
        return true;
    }
    return false;
}

ccmdstr::~ccmdstr()
{
    m_cmdstr.clear();
}


ostream& operator<<(ostream& os,const ccmdstr &p){
    for(int i=0;i<p.size();i++){
        os<<"["<<i<<"]:"<<p[i]<<endl;
    }
    return os;
}

bool getxmlbuffer(const string &xmlbuffer, const string &fieldname, string &value, const int ilen)
{
    if(xmlbuffer.empty()||fieldname.empty()) return false;
    string start ="<"+fieldname+">";
    string end="</"+fieldname+">";
    size_t startpos=xmlbuffer.find(start);
    size_t endpos=xmlbuffer.find(end);
    //cout<<startpos<<","<<endpos;
    if(startpos!=string::npos&&endpos!=string::npos){
        value=xmlbuffer.substr(startpos+start.length(),endpos-startpos-start.length());
        return true;
    }
    return false; 
}

bool getxmlbuffer(const string &xmlbuffer, const string &fieldname, char *value, const int len)
{
    if(xmlbuffer.empty()||fieldname.empty()) return false;
    string start ="<"+fieldname+">";
    string end="</"+fieldname+">";
    size_t startpos=xmlbuffer.find(start);
    size_t endpos=xmlbuffer.find(end);
    if(startpos!=string::npos&&endpos!=string::npos){
        size_t valueStartPos =startpos+start.length();
        size_t valueLength = endpos-startpos-start.length();
        string valueTmp=xmlbuffer.substr(valueStartPos,valueLength);
        if(len>0){
            memset(value,0,len+1);
        }
        strncpy(value,valueTmp.c_str(),valueLength);
        return true;
    }
    return false;
}

bool getxmlbuffer(const string &xmlbuffer, const string &fieldname, bool &value)
{   string str;
    if(getxmlbuffer(xmlbuffer,fieldname,str)==false)return false;
    cout<<str;
    toupper(str);
    
    if(str=="TRUE") {value=true;}
    else {value=false;}
    return true;
    
}

bool getxmlbuffer(const string &xmlbuffer, const string &fieldname, int &value)
{
    string str;
    if(getxmlbuffer(xmlbuffer,fieldname,str)==false)return false;
    try{
        value = stoi(picknumber(str,true,false));
    }
    catch(const std::exception & e){
        std::cerr << "转换失败: " << e.what() 
              << ", 字符串: " << str << std::endl;
        return false;
    }
    return true;
}

bool getxmlbuffer(const string &xmlbuffer, const string &fieldname, unsigned int &value)
{
    string str;
    if(getxmlbuffer(xmlbuffer,fieldname,str)==false)return false;
    try{
        value = stoul(picknumber(str,false,false));
    }
    catch(const std::exception & e){
        std::cerr << "转换失败: " << e.what() 
              << ", 字符串: " << str << std::endl;
        return false;
    }
    return true;
}

bool getxmlbuffer(const string &xmlbuffer, const string &fieldname, long &value)
{
   string str;
    if(getxmlbuffer(xmlbuffer,fieldname,str)==false)return false;
    try{
        value = stol(picknumber(str,true,false));
    }
    catch(const std::exception & e){
        std::cerr << "转换失败: " << e.what() 
              << ", 字符串: " << str << std::endl;
        return false;
    }
    return true;
}

bool getxmlbuffer(const string &xmlbuffer, const string &fieldname, unsigned long &value)
{
    string str;
    if(getxmlbuffer(xmlbuffer,fieldname,str)==false)return false;
    try{
        value = stoul(picknumber(str,false,false));
    }
    catch(const std::exception & e){
        std::cerr << "转换失败: " << e.what() 
              << ", 字符串: " << str << std::endl;
        return false;
    }
    return true;
}

bool getxmlbuffer(const string &xmlbuffer, const string &fieldname, double &value)
{
    string str;
    if(getxmlbuffer(xmlbuffer,fieldname,str)==false)return false;
    try{
        value = stod(picknumber(str,true,true));
    }
    catch(const std::exception & e){
        std::cerr << "转换失败: " << e.what() 
              << ", 字符串: " << str << std::endl;
        return false;
    }
    return true;
}

bool getxmlbuffer(const string &xmlbuffer, const string &fieldname, float &value)
{
    string str;
    if(getxmlbuffer(xmlbuffer,fieldname,str)==false)return false;
    try{
        value = stof(picknumber(str,true,true));
    }
    catch(const std::exception & e){
        std::cerr << "转换失败: " << e.what() 
              << ", 字符串: " << str << std::endl;
        return false;
    }
    return true;
}

/*
  取操作系统的时间（用字符串表示）。
  strtime：用于存放获取到的时间。
  timetvl：时间的偏移量，单位：秒，0是缺省值，表示当前时间，30表示当前时间30秒之后的时间点，-30表示当前时间30秒之前的时间点。
  fmt：输出时间的格式，fmt每部分的含义：yyyy-年份；mm-月份；dd-日期；hh24-小时；mi-分钟；ss-秒，
  缺省是"yyyy-mm-dd hh24:mi:ss"，目前支持以下格式：
  "yyyy-mm-dd hh24:mi:ss"
  "yyyymmddhh24miss"
  "yyyy-mm-dd"
  "yyyymmdd"
  "hh24:mi:ss"
  "hh24miss"
  "hh24:mi"
  "hh24mi"
  "hh24"
  "mi"
  注意：
    1）小时的表示方法是hh24，不是hh，这么做的目的是为了保持与数据库的时间表示方法一致；
    2）以上列出了常用的时间格式，如果不能满足你应用开发的需求，请修改源代码timetostr()函数增加更多的格式支持；
    3）调用函数的时候，如果fmt与上述格式都匹配，strtime的内容将为空。
    4）时间的年份是四位，其它的可能是一位和两位，如果不足两位，在前面补0。
*/
string &ltime(string &strtime, const string &fmt, const int timetvl)
{
    time_t now = time(nullptr);
    time_t timer = now+timetvl;
    timetostr(timer,strtime,fmt);
    return strtime;
}

char *ltime(char *strtime, const string &fmt, const int timetvl)
{
    if(strtime==nullptr) {
        return nullptr;
    }
    time_t now = time(nullptr);
    time_t timer = now+timetvl;
    timetostr(timer,strtime,fmt);
    return strtime;
}

string ltime1(const string &fmt, const int timetvl)
{
    string str;
    ltime(str,fmt,timetvl);
    return str;
}

string &timetostr(const time_t ttime, string &strtime, const string &fmt)
{
    struct tm sttm; localtime_r (&ttime,&sttm);
    sttm.tm_year=sttm.tm_year+1900;                // tm.tm_year成员要加上1900。
    sttm.tm_mon++;                                            // sttm.tm_mon成员是从0开始的，要加1。

    // 缺省的时间格式。
    if ( (fmt=="") || (fmt=="yyyy-mm-dd hh24:mi:ss") )
    {
        strtime=sformat("%04u-%02u-%02u %02u:%02u:%02u",sttm.tm_year,sttm.tm_mon,sttm.tm_mday,\
                   sttm.tm_hour,sttm.tm_min,sttm.tm_sec);
        return strtime;
    }

    if (fmt=="yyyy-mm-dd hh24:mi")
    {
        strtime=sformat("%04u-%02u-%02u %02u:%02u",sttm.tm_year,sttm.tm_mon,sttm.tm_mday,\
                   sttm.tm_hour,sttm.tm_min);
        return strtime;
    }

    if (fmt=="yyyy-mm-dd hh24")
    {
        strtime=sformat("%04u-%02u-%02u %02u",sttm.tm_year,sttm.tm_mon,sttm.tm_mday,sttm.tm_hour);
        return strtime;
    }

    if (fmt=="yyyy-mm-dd")
    {
        strtime=sformat("%04u-%02u-%02u",sttm.tm_year,sttm.tm_mon,sttm.tm_mday); 
        return strtime;
    }

    if (fmt=="yyyy-mm")
    {
        strtime=sformat("%04u-%02u",sttm.tm_year,sttm.tm_mon); 
        return strtime;
    }

    if (fmt=="yyyymmddhh24miss") 
    {
        strtime=sformat("%04u%02u%02u%02u%02u%02u",sttm.tm_year,sttm.tm_mon,sttm.tm_mday,\
                   sttm.tm_hour,sttm.tm_min,sttm.tm_sec);
        return strtime;
    }

    if (fmt=="yyyymmddhh24mi")
    {
        strtime=sformat("%04u%02u%02u%02u%02u",sttm.tm_year,sttm.tm_mon,sttm.tm_mday,\
                   sttm.tm_hour,sttm.tm_min);
        return strtime;
    }

    if (fmt=="yyyymmddhh24")
    {
        strtime=sformat("%04u%02u%02u%02u",sttm.tm_year,sttm.tm_mon,sttm.tm_mday,sttm.tm_hour);
        return strtime;
    }

    if (fmt=="yyyymmdd")
    {
        strtime=sformat("%04u%02u%02u",sttm.tm_year,sttm.tm_mon,sttm.tm_mday); 
        return strtime;
    }

    if (fmt=="hh24miss")
    {
        strtime=sformat("%02u%02u%02u",sttm.tm_hour,sttm.tm_min,sttm.tm_sec); 
        return strtime;
    }

    if (fmt=="hh24mi") 
    {
        strtime=sformat("%02u%02u",sttm.tm_hour,sttm.tm_min); 
        return strtime;
    }

    if (fmt=="hh24")
    {
        strtime=sformat("%02u",sttm.tm_hour); 
        return strtime;
    }

    if (fmt=="mi")
    {
        strtime=sformat("%02u",sttm.tm_min); 
        return strtime;
    }

    return strtime;
}

char *timetostr(const time_t ttime, char *strtime, const string &fmt){   
    if (strtime==nullptr) return nullptr;    // 判断空指针。

    string str;
    timetostr(ttime,str,fmt);           // 直接调用string& timetostr(const time_t ttime,string &strtime,const string &fmt="");
    str.copy(strtime,str.length());
    strtime[str.length()]=0;           // string的copy函数不会给C风格字符串的结尾加0。

    return strtime;
}

string timetostr1(const time_t ttime, const string &fmt)
{
    string str;
    timetostr(ttime,str,fmt);
    return str;
}

time_t strtotime(const string &strtime){
    string strtmp,yyyy,mm,dd,hh,mi,ss;
    picknumber(strtime,strtmp,false,false);
    if(strtmp.length()!=14) {return -1;}
    yyyy=strtmp.substr(0,4);
    mm=strtmp.substr(4,2);
    dd=strtmp.substr(6,2);
    hh=strtmp.substr(8,2);
    mi=strtmp.substr(10,2);
    ss=strtmp.substr(12,2);
    struct tm m_time;
    try{
        m_time.tm_year=stol(yyyy)-1900;
        m_time.tm_mon=stol(mm)-1;
        m_time.tm_mday=stol(dd);
        m_time.tm_hour=stol(hh);
        m_time.tm_min=stol(mi);
        m_time.tm_sec=stol(ss);
    }
    catch(std::exception& e){
        return -1;
    }
    return mktime(&m_time);
}

bool addtime(const string &in_stime, char *out_stime, const int timetvl, const string &fmt)
{
    if(out_stime==nullptr){return false;}
    time_t timer;
    timer = strtotime(in_stime);
    if(timer ==-1){strcpy(out_stime,"");return false;}
    timer +=timetvl;
    timetostr(timer,out_stime,fmt);
    return true;
}

bool addtime(const string &in_stime, string &out_stime, const int timetvl, const string &fmt)
{
    time_t timer;
    timer = strtotime(in_stime);
    if(timer ==-1){out_stime="";return false;}
    timer +=timetvl;
    timetostr(timer,out_stime,fmt);
    return true;
}

ctimer::ctimer()
{
    start();
}

void ctimer::start()
{
    memset(&m_start,0,sizeof(struct timeval));
    memset(&m_end,0,sizeof(struct timeval));

    gettimeofday(&m_start, 0);
}

double ctimer::elapsed()
{
    gettimeofday(&m_end,0);     // 获取当前时间作为计时结束的时间，精确到微秒。

    string str;
    str=sformat("%ld.%06ld",m_start.tv_sec,m_start.tv_usec);
    double dstart=stod(str);      // 把计时开始的时间点转换为double。

    str=sformat("%ld.%06ld",m_end.tv_sec,m_end.tv_usec);
    double dend=stod(str);       // 把计时结束的时间点转换为double。

    start();                                  // 重新开始计时。

    return dend-dstart;
}

// pathorfilename：绝对路径的文件名或目录名。
// bisfilename：指定pathorfilename的类型，true-pathorfilename是文件名，否则是目录名，缺省值为true。
bool newdir(const string &pathorfilename, bool bisfilename)
{
    int pos=1;
    while (true)
    {
        int pos1=pathorfilename.find('/',pos);
        if (pos1==string::npos) break;

        string strpathname=pathorfilename.substr(0,pos1);      // 截取目录。

        pos=pos1+1;
        if (access(strpathname.c_str(),F_OK) != 0)  // 如果目录不存在，创建它。
        {
            // 0755是八进制，不要写成755。
            if (mkdir(strpathname.c_str(),0755) != 0) return false;  // 如果目录不存在，创建它。
        }
    }
    if (bisfilename==false)
    {
        if (access(pathorfilename.c_str(),F_OK) != 0)
        {
            if (mkdir(pathorfilename.c_str(),0755) != 0) return false;
        }
    }
    return true;
}

bool renamefile(const string &oldfilename, const string &newfilename)
{
    if(access(oldfilename.c_str(),F_OK)!=0){
        return false;
    }
    if(newdir(newfilename,true)==false){
        return false;
    }
    if(rename(oldfilename.c_str(),newfilename.c_str())!=0){
        return false;
    }
    return true;
}

bool copyfile(const string &oldfilename, const string &newfilename)
{
    if(access(oldfilename.c_str(),F_OK)!=0){
        return false;
    }
    if(newdir(newfilename,true)==false){
        return false;
    }
    std::ifstream in(oldfilename, std::ios::binary);
    std::ofstream out(newfilename, std::ios::binary);
    if(!in.is_open()||!out.is_open()){
        cerr<<"file can not open"<<endl;
        return false;
    }

    const size_t buffer_size = 4096;
    char buffer[buffer_size];
    while(in.read(buffer, buffer_size) || in.gcount() > 0){
        out.write(buffer, in.gcount());
    }
    in.close();
    out.close();
    
    return true;
    
}

int filesize(const string &filename)
{   
    if(access(filename.c_str(),F_OK)!=0){
        return -1;
    }
    std::ifstream is(filename,std::ios::binary);
    if(!is.is_open()){
        return -1;
    }
    is.seekg(0,std::ios::end);
    return is.tellg();
}

bool filemtime(const string &filename, char *mtime, const string &fmt)
{   
    struct stat fileinfo;//c中存放文件信息的结构体
    if (stat(filename.c_str(), &fileinfo) != 0) {
        return false;  // 文件不存在或无法访问
    }
 
    mtime=timetostr(fileinfo.st_mtim.tv_sec,mtime,fmt); 
    return true;
}

bool filemtime(const string &filename, string &mtime, const string &fmt)
{
    struct stat info;//c中存放文件信息的结构体
    if (stat(filename.c_str(), &info) != 0) {
        return false;  // 文件不存在或无法访问
    }
    mtime=timetostr(info.st_mtim.tv_sec,mtime,fmt); 
    return true;
}

bool setmtime(const string &filename, const string &mtime)
{
    if(strtotime(mtime)==-1){
        return false;
    }
    time_t new_time =strtotime(mtime);
    struct utimbuf new_times;
    new_times.actime = new_time;   // 访问时间
    new_times.modtime = new_time; //修改时间

    if(utime(filename.c_str(), &new_times) != 0) {
        return false;
    }
    return true;
}


bool cdir::opendir(const string &dirname, const string &rules, const int maxfiles, const bool bandchild, bool bsort)
{
    m_filelist.clear();
    m_pos =0;
    if(newdir(dirname,false)==false){//如果目录不存在就创建目录
        cerr<<"newdir error";
        return false;
    }
    bool ret= _opendir(dirname,rules,maxfiles,bandchild);

    //cout<<"ret="<<ret<<endl;
    if(bsort==true){
        sort(m_filelist.begin(),m_filelist.end());
    }
    return ret;
    
}

bool cdir::_opendir(const string &dirname, const string &rules, const int maxfiles, const bool bandchild)
{
    DIR * dir=::opendir(dirname.c_str());
    if(dir==nullptr){
        cerr<<"dir";
        return false;
    }
    
    struct dirent *stdir;
    while((stdir = ::readdir(dir)) != nullptr){
        if(stdir->d_name[0]=='.')   {
            continue;
            }
        if(m_filelist.size()>maxfiles){
           // cout<<"over the maxfiles"<<endl;
            break;
        }
       // cout<<"dirname="<<dirname<<endl;
        //cout<<"dname="<<stdir->d_name<<"|"<<endl;;
        string curfullpathandname= dirname+"/"+stdir->d_name;
       //cout<<"curfullpathandname="<<curfullpathandname<<endl;
        if(stdir->d_type==DT_DIR&&bandchild==true){
            bool isOpen = _opendir(curfullpathandname,rules,maxfiles,bandchild);
            if(isOpen==false){
               // cerr<<"isOpen"<<endl;
                closedir(dir);
                return false;
            }
        }
        if(stdir->d_type==8){
            if(matchstr(stdir->d_name,rules) == false) {
                continue;
            }
            
            m_filelist.push_back(std::move(curfullpathandname));
        }
    }
    closedir(dir);
    return true;
}

bool cdir::readdir()
{
    if (m_pos >= m_filelist.size()) 
    {
        m_pos=0;
        m_filelist.clear();
        return false;
    }
    
    m_ffilename = m_filelist[m_pos];
    m_dirname=m_ffilename.substr(0,m_ffilename.find_last_of("/"));
    m_filename=m_ffilename.substr(m_ffilename.find_last_of("/")+1);
    //cout << "m_ffilename (完整路径): " << m_ffilename << endl;
    //cout << "m_dirname (目录路径): " <<m_dirname << endl;
    //cout << "m_filename (文件名): " << m_filename << endl;

    struct stat filestat;
    if(stat(m_ffilename.c_str(),&filestat)!=0){
        return false; 
    }
    m_filesize=filestat.st_size;                                     // 文件大小。
    m_mtime=timetostr1(filestat.st_mtim.tv_sec,m_fmt); 
    //cout<<m_mtime<<endl;  // 文件最后一次被修改的时间。
    m_ctime=timetostr1(filestat.st_ctim.tv_sec,m_fmt);      // 文件生成的时间。
    m_atime=timetostr1(filestat.st_atim.tv_sec,m_fmt);      // 文件最后一次被访问的时间。
    m_pos++;
    return true;
}

void cdir::setfmt(const string &fmt)
{
    m_fmt=fmt;
}

cdir::~cdir()
{
    m_filelist.clear();
    m_pos=0;
}

bool cofile::open(const string &filename, const bool btmp, const ios::openmode mode, const bool benbuffer)
{
    if(fout.is_open()){
        fout.close();
    }//fout如果指向其他文件，先关掉

    m_filename=filename;
    newdir(filename,true);

    

    if(btmp==true){
       m_filenametmp = filename+".tmp";
       fout.open(m_filenametmp,mode);
    } 
    else{
        m_filename.clear();
        fout.open(filename,mode);
    }
    if(benbuffer==false){
        fout<<unitbuf;
    }
    return fout.is_open();
}

bool cofile::write(void *buf, int bufsize)
{
    if(fout.is_open()==false){
        return false;
    }
    fout.write(static_cast<const char*>(buf),bufsize);
    return fout.good();
}

bool cofile::closeandrename()
{
    
    if(!fout.is_open()){
        return false;
    }
    cout<<m_filenametmp<<endl;
    if(m_filenametmp.empty()==false){
        //cout<<m_filenametmp.length()<<endl;
        int i =rename(m_filenametmp.c_str(),m_filename.c_str());
        //cout<<i<<endl;
        if(i!=0){
            return false;
        }
    }
    fout.close();
    return true;
}

void cofile::close()
{
    if(fout.is_open()==false){
        return ;
    }
    fout.close();
    if(m_filenametmp.length()!=0){
        remove(m_filenametmp.c_str());
    }
}

cofile::~cofile()
{
    close();
}


bool cifile::open(const string &filename, const ios::openmode mode)
{   m_filename=filename;
    if(fin.is_open()){
        close();
    }
    fin.open(m_filename,mode);
    return fin.is_open();
}

bool cifile::readline(string &buf, const string &endbz){
    buf.clear();            // 清空buf。
    string strline;        // 存放从文件中读取的一行。
    while (true)
    {
        getline(fin,strline);    // 从文件中读取一行。
        if (fin.eof()) break;    // 如果文件已读完。
        buf=buf+strline;      // 把读取的内容拼接到buf中。
        if (endbz=="")
            return true;          // 如果行没有结尾标志。
        else 
        {
            // 如果行有结尾标志，判断本次是否读到了结尾标志，如果没有，继续读，如果有，返回。
            if (buf.find(endbz,buf.length()-endbz.length()) != string::npos) return true;
        }
        buf=buf+"\n";        // getline从文件中读取一行的时候，会删除\n，所以，这里要补上\n，因为这个\n不应该被删除。
    }

    return false;

    // if (!fin.is_open()) {   
    //     cerr<<"isnot open"<<endl;
    //     return false;
    //     }
    // buf.clear();
    // string line;
    // while(getline(fin,line)){
        
    //     size_t pos = line.find(endbz);
    //     if(pos==string::npos){
    //         buf+=line+'\n';
    //     }
    //     else{ 
    //         string str=line.substr(0,pos);
    //         buf+=str;
    //         return true;
    //     }
    // }
    // if(endbz==""){
    //     return true;
    // }
    // return false;
}

int cifile::read(void *buf, const int bufsize)
{
    if(!fin.is_open()){return 0;}
    fin.read(static_cast<char*>(buf),static_cast<streamsize>(bufsize));
    return fin.gcount();
}

bool cifile::closeandremove()
{
    if (fin.is_open()==false){return false;} 
    fin.close(); 
    if (remove(m_filename.c_str())!=0) {return false;}
    return true;
}

void cifile::close()
{
    if(!fin.is_open()){
        return;
    }
    fin.close();
}
// 打开日志文件。
// filename：日志文件名，建议采用绝对路径，如果文件名中的目录不存在，就先创建目录。
// mode：日志文件的打开模式，缺省值是ios::app。
// backup：是否自动切换（备份），true-切换，false-不切换，在多进程的服务程序中，如果多个进程共用一个日志文件，bbackup必须为false。
// enbuffer：是否启用文件缓冲机制，true-启用，false-不启用，如果启用缓冲区，那么写进日志文件中的内容不会立即写入文件，缺省是不启用。
// bool clogfile::open(const string &filename, const std::ios::openmode mode, const bool backup, const bool enbuffer)
// {
//     if(fout.is_open()){
//         fout.close();
//     }
//     m_filename=filename;
//     m_backup=backup;
//     m_enbuffer=enbuffer;
//     m_mode=mode;

//     newdir(filename,true);
//     fout.open(m_filename,mode);
//     if (m_enbuffer==false) fout << unitbuf;       // 是否启用文件缓冲区。
//     return fout.is_open();
// }

// bool clogfile::backup()
// {
//     if (!fout.is_open()) {
//         cout << "Error: log file is not open!" << endl;
//         return false;
//     }
//     if(m_backup==false){
//         return true;
//         }
//     cout<<"tellp:"<<fout.tellp();
//     if(fout.tellp()>m_maxsize*1024*1024){
//         m_splock.lock();
//         fout.close();
//         string logfilename=m_filename+ltime1();
//         rename(m_filename.c_str(),logfilename.c_str());
//         if (m_enbuffer==false) fout << unitbuf; 
//         fout.open(m_filename,m_mode);
//         m_splock.unlock();
//     }
//     return fout.good();
// }
bool clogfile::open(const string &filename,const ios::openmode mode,const bool bbackup,const bool benbuffer)
{
    // 如果日志文件是打开的状态，先关闭它。
    if (fout.is_open()) fout.close();

    m_filename=filename;        // 日志文件名。
    m_mode=mode;                 // 打开模式。
    m_backup=bbackup;          // 是否自动备份。
    m_enbuffer=benbuffer;      // 是否启用文件缓冲区。

    newdir(m_filename,true);                              // 如果日志文件的目录不存在，创建它。

    fout.open(m_filename,m_mode);                  // 打开日志文件。

    if (m_enbuffer==false) fout << unitbuf;       // 是否启用文件缓冲区。

    return fout.is_open();
}

bool clogfile::backup()
{
    // 不备份
    if (m_backup == false) return true;

    if (fout.is_open() == false) return false;

    // 如果当前日志文件的大小超过m_maxsize，备份日志。
    if (fout.tellp() > m_maxsize*1024*1024)
    {
        m_splock.lock();       // 加锁。

        fout.close();              // 关闭当前日志文件。

        // 拼接备份日志文件名。
        string bak_filename=m_filename+"."+ltime1("yyyymmddhh24miss");

        rename(m_filename.c_str(),bak_filename.c_str());   // 把当前日志文件改名为备份日志文件。

        fout.open(m_filename,m_mode);              // 重新打开当前日志文件。

        if (m_enbuffer==false) fout << unitbuf;   // 判断是否启动文件缓冲区。

        m_splock.unlock();   // 解锁。

        return fout.is_open();
    }

    return true;
}



bool ctcpclient::connect(const string& ip, const int port)
{
    if(m_connectfd!=-1){return false;}
    
    m_connectfd=socket(AF_INET,SOCK_STREAM,0);
    if(m_connectfd==-1)return false;
    m_ip =ip;
    m_port=port;

    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family=PF_INET;
    addr.sin_port=htons(m_port);
    struct hostent *h;//获取IP地址然后存到addr里
    h= gethostbyname(m_ip.c_str());
    if(h==0){
        cerr << "gethostbyname error: " << hstrerror(h_errno) << endl;
        close(m_connectfd);
        m_connectfd=-1;
        return false;
        }
    memcpy(&addr.sin_addr,h->h_addr_list[0],h->h_length);
    
    if(::connect(m_connectfd,(struct sockaddr *)&addr,socklen_t(sizeof(struct sockaddr_in)))<0){
        cerr<<"connect error"<<strerror(errno)<<endl;
        close(m_connectfd);
        m_connectfd=-1;
        return false;
    }
    cout << "Connected to " << m_ip << ":" << m_port << " successfully" << endl;
    return true;
}

bool ctcpclient::write(const string &buffer)
{
    if(m_connectfd<0)   {return false;}
    return tcpwrite(m_connectfd,buffer);
}

bool ctcpclient::write(const char *buffer, const int size)
{
   if(m_connectfd<0)   {return false;}
   return tcpwrite(m_connectfd,buffer,size);
}

bool tcpwrite(const int connectfd, const string &buffer)
{
    if(connectfd<0){return false;}
    int buffsize=buffer.length();
    if(!writen(connectfd,static_cast<const void*>(&buffsize),4))return false;
    if(!writen(connectfd,buffer.data(),buffsize)) return false;
    return true;
}

bool tcpwrite(const int connectfd, const char *buffer, const int size)
{
    if(connectfd<0){return false;}
    if(!writen(connectfd,buffer,size)) return false;
    return true;
}

bool writen(const int connectfd, const void *buffer, const int size)
{
    int nleft = size;//剩余要发的
    ssize_t in=0;//已经发送的
    int nwritten;//这次成功发送的
    while(nleft>0){
        nwritten = send(connectfd,buffer,nleft,0);
        in+=nwritten;
        nleft-=nwritten;
    } 
    return true;
}


bool ctcpclient::read(string& buffer, int itimeout)
{ 
    if(m_connectfd<0)   {return false;}
   return tcpread(m_connectfd,buffer,itimeout);
}

bool ctcpclient::read(char *buffer, int maxsize, const int itimeout)
{
    if(m_connectfd<0)   {return false;}
    return tcpread(m_connectfd,buffer,maxsize,itimeout);
}

bool tcpread(const int connectfd, string &buffer, const int itimeout)
{
    if (connectfd==-1) return false;

    // 如果itimeout>0，表示等待itimeout秒，如果itimeout秒后接收缓冲区中还没有数据，返回false。
    if (itimeout>0)
    {
        struct pollfd fds;
        fds.fd=connectfd;
        fds.events=POLLIN;
        if ( poll(&fds,1,itimeout*1000) <= 0 ) return false;
    }

    // 如果itimeout==-1，表示不等待，立即判断socket的接收缓冲区中是否有数据，如果没有，返回false。
    if (itimeout==-1)
    {
        struct pollfd fds;
        fds.fd=connectfd;
        fds.events=POLLIN;
        if ( poll(&fds,1,0) <= 0 ) return false;
    }

    int buflen=0;

    // 先读取报文长度，4个字节。
    if (readn(connectfd,(char*)&buflen,4) == false) return false;

    buffer.resize(buflen);   // 设置buffer的大小。

    // 再读取报文内容。
    if (readn(connectfd,&buffer[0],buflen) == false) return false;

    return true;

}

bool tcpread(const int connectfd, char *buffer, const int size, const int itimeout)
{
    if(connectfd==-1)   {return false;}
    // 如果itimeout>0，表示等待itimeout秒，如果itimeout秒后接收缓冲区中还没有数据，返回false。
    if (itimeout>0)
    {
        struct pollfd fds;
        fds.fd=connectfd;
        fds.events=POLLIN;
        if ( poll(&fds,1,itimeout*1000) <= 0 ) return false;
    }

    // 如果itimeout==-1，表示不等待，立即判断socket的接收缓冲区中是否有数据，如果没有，返回false。
    if (itimeout==-1)
    {
        struct pollfd fds;
        fds.fd=connectfd;
        fds.events=POLLIN;
        if ( poll(&fds,1,0) <= 0 ) return false;
    }
    
    if(readn(connectfd,buffer,size)==false){return false;}
    return true;

}

bool readn(const int connectfd, char *buffer, const int size)
{
    int total_read = 0;
    //cout<<"size="<<size<<endl;
    while (total_read < size) {
        int n = recv(connectfd, buffer + total_read, size - total_read, 0);
        
        if (n < 0) {
            if (errno == EINTR) continue;  // 被信号中断，重试
            cerr << "ReCvIvE failed: " << strerror(errno) << endl;
            return false;
        }
        if (n == 0) {  // 对方关闭连接
            cerr << "Connection closed by peer" << endl;
            return false;
        }
        total_read += n;
        //cout<<<<"|";
        //cout<<"n="<<n<<endl;
        //cout<<"t="<<total_read<<endl;
    }

    return true;
}

char *ctcpserver::getid()
{
        static char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientfd.sin_addr, ip_str, INET_ADDRSTRLEN);
        return ip_str;
}

bool ctcpserver::initserver(const int port, const int backlog)
{
    
    if(m_listenfd!=-1)return false;
    m_listenfd=socket(AF_INET,SOCK_STREAM,0);
    int opt = 1; 
    setsockopt(m_listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt)); 
    serverfd.sin_family=PF_INET;
    serverfd.sin_addr.s_addr=htonl(INADDR_ANY);
    serverfd.sin_port=htons(port);
    //cout<<serverfd.sin_port<<endl;
    //cout<<m_listenfd<<endl;
    if(bind(m_listenfd,(struct sockaddr*)&serverfd,socklen_t(sizeof(struct sockaddr_in)))!=0){
        cout << "bind failed with error " << errno << ": " << strerror(errno) << endl;
        closelistenfd();
        return false;
    }
    if(listen(m_listenfd,backlog)<0){
        cerr<<"listen";
        closelistenfd();
        return false;
    }
    
    cout<<"server is ready,waitting thc client..."<<endl;
    return true;
}

bool ctcpserver::accept()
{
    if(m_listenfd==-1)return false;
    socklen_t clientlen = sizeof(clientfd);
    m_connectfd=::accept(m_listenfd,(struct sockaddr*)&clientfd,&clientlen);
    if(m_connectfd<0)return false;
    cout<<getid()<<" has connecting\n";
    return true;
}

bool ctcpserver::write(const string &buffer)
{
    if(m_connectfd==-1) return false;
    return tcpwrite(m_connectfd,buffer);
}

bool ctcpserver::write(const char *buffer, const int size)
{
    if(m_connectfd==-1) return false;
    return tcpwrite(m_connectfd,buffer);
}


bool ctcpserver::read(string &buffer, const int itimeout)
{
    if(m_connectfd==-1)return false;
    return tcpread(m_connectfd,buffer,itimeout);
}

bool ctcpserver::read(char *buffer, int maxsize, const int itimeout)
{
    if(m_connectfd==-1)  return false;
    return tcpread(m_connectfd,buffer,itimeout);
}

bool idc::ctcpserver::closeconnectfd()
{
    if(m_connectfd>=0){
        ::close(m_connectfd);
        m_connectfd=-1;
        return true;
    }
    return false;
}

bool ctcpserver::closelistenfd()
{
    if(m_listenfd>=0){
        ::close(m_listenfd);
        m_listenfd=-1;
        return true;
    }
    return false;
}

void closeioandsignal(bool bcloseio)
{
    int ii=0;

    for (ii=0;ii<64;ii++)
    {
        if (bcloseio==true) ::close(ii);

        signal(ii,SIG_IGN); 
    }
}


// 如果信号量已存在，获取信号量；如果信号量不存在，则创建它并初始化为value。
// 如果用于互斥锁，value填1，sem_flg填SEM_UNDO。
// 如果用于生产消费者模型，value填0，sem_flg填0。
bool csemp::init(key_t key,unsigned short value,short sem_flg)
{
    if (m_semid!=-1) return false; // 如果已经初始化了，不必再次初始化。

    m_sem_flg=sem_flg;

    // 信号量的初始化不能直接用semget(key,1,0666|IPC_CREAT)
    // 因为信号量创建后，初始值是0，如果用于互斥锁，需要把它的初始值设置为1，
    // 而获取信号量则不需要设置初始值，所以，创建信号量和获取信号量的流程不同。

    // 信号量的初始化分三个步骤：
    // 1）获取信号量，如果成功，函数返回。
    // 2）如果失败，则创建信号量。
    // 3) 设置信号量的初始值。

    // 获取信号量。
    if ( (m_semid=semget(key,1,0666)) == -1)
    {
        // 如果信号量不存在，创建它。
        if (errno==ENOENT)
        {
            // 用IPC_EXCL标志确保只有一个进程创建并初始化信号量，其它进程只能获取。
            if ( (m_semid=semget(key,1,0666|IPC_CREAT|IPC_EXCL)) == -1)
            {
                if (errno==EEXIST) // 如果错误代码是信号量已存在，则再次获取信号量。
                {
                    if ( (m_semid=semget(key,1,0666)) == -1)
                    { 
                        perror("init 1 semget()"); return false; 
                    }
                    return true;
                }
                else  // 如果是其它错误，返回失败。
                {
                    perror("init 2 semget()"); return false;
                }
            }

            // 信号量创建成功后，还需要把它初始化成value。
            union semun sem_union;
            sem_union.val = value;   // 设置信号量的初始值。
            if (semctl(m_semid,0,SETVAL,sem_union) <  0) 
            { 
                perror("init semctl()"); return false; 
            }
        }
        else
        { perror("init 3 semget()"); return false; }
    }

    return true;
}

// 信号量的P操作（把信号量的值减value），如果信号量的值是0，将阻塞等待，直到信号量的值大于0。
bool csemp::wait(short value)
{
    if (m_semid==-1) return false;

    struct sembuf sem_b;
    sem_b.sem_num = 0;      // 信号量编号，0代表第一个信号量。
    sem_b.sem_op = value;   // P操作的value必须小于0。
    sem_b.sem_flg = m_sem_flg;
    if (semop(m_semid,&sem_b,1) == -1) { perror("p semop()"); return false; }

    return true;
}

// 信号量的V操作（把信号量的值减value）。
bool csemp::post(short value)
{
    if (m_semid==-1) return false;

    struct sembuf sem_b;
    sem_b.sem_num = 0;     // 信号量编号，0代表第一个信号量。
    sem_b.sem_op = value;  // V操作的value必须大于0。
    sem_b.sem_flg = m_sem_flg;
    if (semop(m_semid,&sem_b,1) == -1) { perror("V semop()"); return false; }

    return true;
}

// 获取信号量的值，成功返回信号量的值，失败返回-1。
int csemp::getvalue()
{
    return semctl(m_semid,0,GETVAL);
}

// 销毁信号量。
bool csemp::destroy()
{
    if (m_semid==-1) return false;

    if (semctl(m_semid,0,IPC_RMID) == -1) { perror("destroy semctl()"); return false; }

    return true;
}

csemp::~csemp()
{
}


// cpactive::cpactive()
// {
//     m_shmid =-1;//共享内存id
//     m_shm = nullptr;//指向共享内存地址的指针
//     m_pos =-1;//共享内存内部的第x个地址
// }

// bool cpactive::addpinfo(const int timeout, const string &pname, clogfile *logfile)
// {
//     csemp semlock;
//     if(semlock.init(0x5005)==false)
//     {
//         cout<<"创建信号量失败\n";
//         return false;
//     }
//     //返回共享内存id，参数是地址，大小，方式
//     m_shmid = shmget(0x5005,1000*sizeof(st_procinfo),0666|IPC_CREAT);
//     if(m_shmid==-1){
//         if(logfile!=nullptr)  logfile->write("创建/获取共享内存(%x)失败。\n",0x5005); 
//         else    perror("shmget failed");
//         return false;
//     }

//     semlock.wait();
//     //把进程和共享内存连接
//     m_shm = (st_procinfo *)shmat(m_shmid, NULL, 0);
//     if (m_shm == (void *)-1) {
//         if(logfile!=nullptr)  logfile->write("shmat failed"); 
//         else    perror("shmat failed");
//        return false;
//     }
//     //要保存的结构体
//     st_procinfo procinfo(getpid(),pname,timeout,time(nullptr));

//     for(int i=0;i<1000;i++){
//         if(m_shm[i].pid==procinfo.pid){
//             m_pos=i;
//             cout<<"找到旧位置"<<m_pos<<endl;
//             break;
//         }
//     }
//     if(m_pos==-1){
//         for(int i=0;i<1000;i++){
//             if(m_shm[i].pid==0){
//                 m_pos=i;
//                 cout<<"找到新位置"<<m_pos<<endl;
//                 break;
//             }
//         }
//     }
    
//     if(m_pos==-1){
//         semlock.post();
//         perror("共享空间已用完。\n");
//         return false;
//     }
//     memcpy(&m_shm[m_pos],&procinfo,sizeof(st_procinfo));
//     semlock.post();
//     return true;
//     //调试代码
//     //  for(int i=0;i<1000;i++){
//     //     if(m_shm[i].pid!=0){
            
//     //         cout<<"i="<<i<<
//     //         ",pid="<<m_shm[i].pid<<
//     //         ",pname="<<m_shm[i].pname<<
//     //         ",timeout="<<m_shm[i].timeout<<
//     //         ",atime="<<m_shm[i].atime<<endl;
//     //     }
//     // }
// }

// bool cpactive::uptatime()
// {
//     if(m_shmid==-1)return false;
//     //更新心跳信息
//     m_shm[m_pos].atime=time(nullptr);
//     return true;
// }

// cpactive::~cpactive()
// {
//     if(m_pos!=-1){
//         memset(m_shm+m_pos,0,sizeof(st_procinfo));
//     }
//     if (shmdt(m_shm) !=0) {
//         perror("shmdt failed");
//     }
// }

cpactive::cpactive()
{
     m_shmid=0;
     m_pos=-1;
     m_shm=0;
}

bool cpactive::addpinfo(const int timeout, const string &pname, clogfile *logfile)
{
    if (m_pos!=-1) return true;

    // 创建/获取共享内存，键值为SHMKEYP，大小为MAXNUMP个st_procinfo结构体的大小。
    if ( (m_shmid = shmget((key_t)SHMKEYP, MAXNUMP*sizeof(struct st_procinfo), 0666|IPC_CREAT)) == -1)
    { 
        if (logfile!=nullptr) logfile->write("创建/获取共享内存(%x)失败。\n",SHMKEYP); 
        else printf("创建/获取共享内存(%x)失败。\n",SHMKEYP);

        return false; 
    }

    // 将共享内存连接到当前进程的地址空间。
    m_shm=(struct st_procinfo *)shmat(m_shmid, 0, 0);
  
    /*
    struct st_procinfo stprocinfo;    // 当前进程心跳信息的结构体。
    memset(&stprocinfo,0,sizeof(stprocinfo));
    stprocinfo.pid=getpid();            // 当前进程号。
    stprocinfo.timeout=timeout;         // 超时时间。
    stprocinfo.atime=time(0);           // 当前时间。
    strncpy(stprocinfo.pname,pname.c_str(),50); // 进程名。
    */
    st_procinfo stprocinfo(getpid(),pname.c_str(),timeout,time(0));    // 当前进程心跳信息的结构体。

    // 进程id是循环使用的，如果曾经有一个进程异常退出，没有清理自己的心跳信息，
    // 它的进程信息将残留在共享内存中，不巧的是，如果当前进程重用了它的id，
    // 守护进程检查到残留进程的信息时，会向进程id发送退出信号，将误杀当前进程。
    // 所以，如果共享内存中已存在当前进程编号，一定是其它进程残留的信息，当前进程应该重用这个位置。
    for (int ii=0;ii<MAXNUMP;ii++)
    {
        if ( (m_shm+ii)->pid==stprocinfo.pid ) { m_pos=ii; break; }
    }

    csemp semp;                       // 用于给共享内存加锁的信号量id。

    if (semp.init(SEMKEYP) == false)  // 初始化信号量。
    {
        if (logfile!=nullptr) logfile->write("创建/获取信号量(%x)失败。\n",SEMKEYP); 
        else printf("创建/获取信号量(%x)失败。\n",SEMKEYP);

        return false;
    }

    semp.wait();  // 给共享内存上锁。

    // 如果m_pos==-1，表示共享内存的进程组中不存在当前进程编号，那就找一个空位置。
    if (m_pos==-1)
    {
        for (int ii=0;ii<MAXNUMP;ii++)
            if ( (m_shm+ii)->pid==0 ) { m_pos=ii; break; }
    }

    // 如果m_pos==-1，表示没找到空位置，说明共享内存的空间已用完。
    if (m_pos==-1) 
    { 
        if (logfile!=0) logfile->write("共享内存空间已用完。\n");
        else printf("共享内存空间已用完。\n");

        semp.post();  // 解锁。

        return false; 
    }

    // 把当前进程的心跳信息存入共享内存的进程组中。
    memcpy(m_shm+m_pos,&stprocinfo,sizeof(struct st_procinfo)); 

    semp.post();   // 解锁。

    return true;
}

bool cpactive::uptatime()
{
    if (m_pos==-1) return false;

    (m_shm+m_pos)->atime=time(0);

    return true;
}

cpactive::~cpactive()
{
     // 把当前进程从共享内存的进程组中移去。
    if (m_pos!=-1) memset(m_shm+m_pos,0,sizeof(struct st_procinfo));

    // 把共享内存从当前进程中分离。
    if (m_shm!=0) shmdt(m_shm);
}

} // namespace