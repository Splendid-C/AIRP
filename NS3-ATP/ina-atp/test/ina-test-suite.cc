
// Include a header file from your module to test.

// An essential include is test.h
#include "ns3/test.h"
#include "ns3/ina-aggregator-application.h"
#include "ns3/uinteger.h"

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

// Add a doxygen group for tests.
// If you have more than one test, this should be in only one of them.
/**
 * \defgroup ina-tests Tests for ina
 * \ingroup ina
 * \ingroup tests
 */

// This is an example TestCase.
/**
 * \ingroup ina-tests
 * Test case for feature 1
 */
class InaTestCase1 : public TestCase
{
  public:
    InaTestCase1();
    virtual ~InaTestCase1();

  private:
    void DoRun() override;
};

// Add some help text to this case to describe what it is intended to test
InaTestCase1::InaTestCase1()
    : TestCase("Ina test case (does nothing)")
{
}

// This destructor does nothing but we include it as a reminder that
// the test case should clean up after itself
InaTestCase1::~InaTestCase1()
{
}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
InaTestCase1::DoRun()
{
    // A wide variety of test macros are available in src/core/test.h
    //NS_TEST_ASSERT_MSG_EQ(true, true, "true doesn't equal true for some reason");
    // Use this one for floating point comparisons
    //NS_TEST_ASSERT_MSG_EQ_TOL(0.01, 0.01, 0.001, "Numbers are not equal within tolerance");

    InaAggregator inaAggregator;
    std::cout << "inaAggregator.m_aggAlu.size() = " << inaAggregator.m_aggAlu.size() << std::endl;
    NS_TEST_ASSERT_MSG_EQ(0, inaAggregator.m_aggAlu.size(), "not initialized, shoule be 0");
    inaAggregator.Initialize();
    NS_TEST_ASSERT_MSG_EQ(2, inaAggregator.m_aggAlu.size(), "2");
    
}

// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined

/**
 * \ingroup ina-tests
 * TestSuite for module ina
 */
class InaTestSuite : public TestSuite
{
  public:
    InaTestSuite();
};

InaTestSuite::InaTestSuite()
    : TestSuite("ina-test", UNIT)
{
    // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
    AddTestCase(new InaTestCase1, TestCase::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
/**
 * \ingroup ina-tests
 * Static variable for test initialization
 */
static InaTestSuite sinaTestSuite;
