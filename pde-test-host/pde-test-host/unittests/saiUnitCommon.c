/*********************************************************************
 *
 * Broadcom Proprietary and Confidential. (C) Copyright 2016-2018 Broadcom.
 * All rights reserved.
 *
 **********************************************************************
 *
 * @filename  saiUnitCommon.c
 *
 * @purpose   This file contains the common code for unit tests
 *
 * @component unit test
 *
 * @comments
 *
 * @create    20 Jan 2017
 *
 * @end
 *
 **********************************************************************/
#include "saiUnitUtil.h"

#include <time.h>

extern int saiSonicPdeAdd(void);
extern CU_pSuite pSonicPdeSuite;

extern CU_pSuite f_pRunningSuite;
extern CU_ErrorCode basic_initialize(void);
extern void console_registry_level_run(CU_pTestRegistry pRegistry);
static void logging_test_start_message_handler(const CU_pTest pTest, const CU_pSuite pSuite);
static void logging_test_complete_message_handler(const CU_pTest pTest, const CU_pSuite pSuite, const CU_pFailureRecord pFailureList);

static void logging_all_tests_complete_message_handler(const CU_pFailureRecord pFailure);
typedef int (*saiUnitTestAddf)(void);

struct
{
   saiTestId_t      testId;
   char             *testName;
   saiUnitTestAddf  testFunc;
}  saiUnitTestModules[] =
{
   { SAI_TEST_ID_ALL,                    "All",                          NULL },
   { SAI_TEST_ID_PDE,                    "Sonic PDE Tests",              saiSonicPdeAdd  },
};

const char *saiUnitTestModuleNameGet(saiTestId_t testId)
{
   if ((testId <  SAI_TEST_ID_ALL) ||
       (testId >= SAI_TEST_ID_MAX))
   {
       return NULL;
   }
   return saiUnitTestModules[testId].testName;
}

int saiUnitTestsRun(saiTestType_t testType, int testRunQA, saiTestId_t testId)
{
   int i;

   if (CUE_SUCCESS != CU_initialize_registry())
   {
       return CU_get_error();
   }

   if (SAI_TEST_ID_ALL == testId)
   {
       for (i = SAI_TEST_ID_MIN; i < SAI_TEST_ID_MAX; i++)
       {
           if (0 != saiUnitTestModules[i].testFunc())
           {
               printf("\r\nFailed to add %s Unit tests.  Aborting.\r\n", saiUnitTestModules[i].testName);
               return -1;
           }
       }
   }
   else if ((testId >= SAI_TEST_ID_MIN) &&
            (testId <  SAI_TEST_ID_MAX))
   {
       if (0 != saiUnitTestModules[testId].testFunc())
       {
           printf("\r\nFailed to add %s Unit tests.  Aborting.\r\n", saiUnitTestModules[testId].testName);
           return -1;
       }
   }
   else
   {
       printf("\r\nInvalid test id (%d) specified.  Aborting.\r\n", testId);
   }

   switch (testType)
   {
       case SAI_TEST_BASIC:
           CU_basic_set_mode(CU_BRM_VERBOSE);
           CU_set_suite_active(pSonicPdeSuite, CU_FALSE);
           CU_basic_run_tests();
           CU_basic_show_failures(CU_get_failure_list());
           break;

       case SAI_TEST_AUTOMATED:
           CU_set_output_filename("/tmp/CUnitAutomated");
           CU_set_suite_active(pSonicPdeSuite, CU_TRUE);
           CU_automated_run_tests();
           CU_list_tests_to_file();
           break;
           
       case SAI_TEST_CONSOLE:
           CU_console_run_tests();
           break;
           /* To be used to save the test results in case of
              crashes/switch disconects etc */
       case SAI_TEST_LOGGING:
           CU_basic_set_mode(CU_BRM_VERBOSE);
           basic_initialize();          
           CU_set_test_start_handler(logging_test_start_message_handler);
           /* Print to file the test assertions after each test run! */
           CU_set_test_complete_handler(logging_test_complete_message_handler);
           /* Print test summary out as well at the end */
           CU_set_all_test_complete_handler(logging_all_tests_complete_message_handler);
           console_registry_level_run(NULL);
           break;

       default:
            return -1;
   }

   printf("\r\n");
   CU_cleanup_registry();
   return CU_get_error();
}

static  char logFileName[256];
static  char* saiFileBase = "/tmp/SAIlogs/SAI_";
static FILE* fp = NULL;

void set_logging_file_name(char* testName)
{
     memset(logFileName, '\0', sizeof(logFileName));
     strcpy(logFileName, saiFileBase);
     strncat(logFileName, testName, 128);
}

/*------------------------------------------------------------------------*/
/** Handler function called at start of each test.
 *  @param pTest  The test being run.
 *  @param pSuite The suite containing the test.
 */
void logging_test_start_message_handler(const CU_pTest pTest, const CU_pSuite pSuite)
{
  assert(NULL != pSuite);
  assert(NULL != pTest);
  
  assert(NULL != pTest->pName);
  if ((NULL == f_pRunningSuite) || (f_pRunningSuite != pSuite)) {
      assert(NULL != pSuite->pName);
      fprintf(stdout, "\n%s: %s", "Suite", pSuite->pName);
      fprintf(stdout, "\n  %s: ...Test", pTest->pName);    
      f_pRunningSuite = pSuite;   
  }
  else {
      fprintf(stdout, "\n  %s: ...Test", pTest->pName);
  }
   
}


static void logging_all_tests_complete_message_handler(const CU_pFailureRecord pFailure)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char timestr[128];
    sprintf(timestr, "now: %d-%d-%d %d:%d:%d\n", 
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    
    CU_UNREFERENCED_PARAMETER(pFailure); /* not used in basic interface */
    printf("\n\n");
    CU_print_run_results(stdout);
    /* Also print to file */
    fp = NULL;
    system("mkdir -p /tmp/SAIlogs");
    fp = fopen("/tmp/SAIlogs/AllResultsSummary", "a");
    if (fp == NULL)
    {
        fprintf(stdout,"Error getting log file %s\n", logFileName);
    }
    else
    {        
        fprintf(fp,"\n====================================\n");
        fprintf(fp, "\n\nTest results at %s\n\n", timestr);
        fprintf(fp,"\n====================================\n");
        CU_print_run_results(fp);  
    }
    printf("\n");
    if (fp != NULL)
    {
        fclose(fp);
    }
}

/*------------------------------------------------------------------------*/
/** Handler function called at completion of each test.
 *  @param pTest   The test being run.
 *  @param pSuite  The suite containing the test.
 *  @param pFailure Pointer to the 1st failure record for this test.
 */
 void logging_test_complete_message_handler(const CU_pTest pTest, 
                                            const CU_pSuite pSuite, 
                                            const CU_pFailureRecord pFailureList)
{
  CU_pFailureRecord pFailure = pFailureList;
  int i;
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  char timestr[128];
  sprintf(timestr, "now: %d-%d-%d %d:%d:%d\n", 
          tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

  assert(NULL != pSuite);
  assert(NULL != pTest);
  fp = NULL;
 
  if (NULL == pFailure) {
      fprintf(stdout, "\t...passed");     
  }
  else {
      /* Only write test files if they fail */
      set_logging_file_name(pTest->pName);
      system("mkdir -p /tmp/SAIlogs");
      fp = fopen(logFileName, "w");
      if (fp == NULL)
      {
          fprintf(stdout,"Error getting log file %s\n", logFileName);
      }
      fprintf(stdout, "FAILED");
      fprintf(stdout, "\nSuite %s, Test %s had failures:", pSuite->pName, pTest->pName);
      if (fp != NULL)
      {
          fprintf(fp,"FAILED");
          fprintf(fp, "\nTime %s Suite %s, Test %s had failures:", 
                  timestr,
                  pSuite->pName, pTest->pName);
      }
    }
  if (fp != NULL) {
      for (i = 1 ; (NULL != pFailure) ; pFailure = pFailure->pNext, i++) {                  
          fprintf(fp, "\n    %d. %s:%u  - %s", i,
                  (NULL != pFailure->strFileName) ? pFailure->strFileName : "",
                  pFailure->uiLineNumber,
                  (NULL != pFailure->strCondition) ? pFailure->strCondition : "");
          
      }
  }
  if (fp!=NULL)
  {
      fclose(fp);
  }  
}
