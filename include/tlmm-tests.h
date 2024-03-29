#ifndef __TLMM_TESTS_H__
#define __TLMM_TESTS_H__

enum Response
{
    SUCCESS,
    FAILURE,
    TIMEOUT,
};

class _Test
{
public:
    virtual void Init()                = 0;
    virtual void Cleanup()             = 0;
    virtual Response Test(float& time) = 0;

    static void RunTests(const char* group = 0, int runs = 0);
    
protected:
    void Add(const char* name, const char* group, float time);

private:
    struct _test
    {
	char*  name;
	_Test* test;
	float  time;
    };
    struct _group
    {
	char* name;
	_test* tests;
    };
    struct _result
    {
	float time;
	int   success;
	int   fail;
    _result() : time(0.0f), success(0), fail(0) {}
    };

    static void RunGroup(_group* group, _result* res, int runs);
    static void RunTest(_test* test, _result* res, int runs);
    static _group* s_Tests;
};

#include <sys/time.h>
extern struct timeval tv_start;
inline float _time(struct timeval time)
{
    float t = (float)(time.tv_sec - tv_start.tv_sec) * 1000.f;
    t += (float)time.tv_usec / 1000.f;
    return t;
}

#define TEST(x, grp, time, init, cleanup, test, data)	\
    class x : public _Test				\
    {							\
    private:						\
	struct _data data;				\
	_data m_data;					\
    public:						\
	x() { Add(#x, #grp, time); }			\
	void Init() {init}				\
	void Cleanup() {cleanup}			\
	Response Test(float& t)				\
	{						\
	    struct timeval start, end;			\
	    gettimeofday(&start, NULL);			\
	    {test}					\
	    gettimeofday(&end, NULL);			\
	    float dt = _time(end) - _time(start);	\
	    if(t != 0.0f && t < dt)			\
	    {						\
		t = dt;					\
		return TIMEOUT;				\
	    }						\
	    t = _time(end) - _time(start);		\
	    return SUCCESS;				\
	}						\
    };          					\
    x _##x;

#define TEST_TLMM(x, grp, time, fn, test)				\
    TEST(x, grp, time,							\
	 { m_data.program = tlmmInitProgram(); tlmmParseProgram(m_data.program, fn); }, \
	 {tlmmTerminateProgram(&m_data.program);},			\
	 test,								\
	 { tlmmProgram* program; });

#define ASSERT(x)				\
    if(!(x))					\
    {						\
	struct timeval now;			\
	gettimeofday(&now, NULL);		\
	t = _time(now) - _time(start);		\
	return FAILURE;				\
    }

#define SAFE_DELETE(x)				\
    if(x)					\
    { delete x; x = 0; }

#endif/*__TLMM_TESTS_H__*/
