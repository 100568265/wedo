#include <iostream>

using namespace std;

int main()
{
    cout << "Hello world!" << endl;

    time_t now;

    struct tm  *ts;
    char yearchar[80];

    now = time(NULL);
    ts = localtime(&now);
    ts->tm_mday = ts->tm_mday-0;
    mktime(ts); /* Normalise ts */
    strftime(yearchar, sizeof(yearchar), "%Y%m%d", ts);
    cout<<"yeatchar:"<<yearchar<<endl;

    return 0;
}
