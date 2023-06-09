#ifndef HTTP_H
#define HTTP_H

#define MY_HTTP_DEFAULT_PORT 80
class http
{
    public:
        http();
        virtual ~http();
        char * http_get(const char *url);
        char * http_post(const char *url,const char * post_str);
    protected:
    private:

};

#endif // HTTP_H
