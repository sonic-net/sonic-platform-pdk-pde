/*********************************************************************
 *
 * Broadcom Proprietary and Confidential. (C) Copyright 2016-2018 Broadcom.
 * All rights reserved.
 *
 **********************************************************************
 *
 * @filename  saiUnitTest.h
 *
 * @purpose   This file contains the definitions for invoking the unit tests
 *
 * @component unit test
 *
 * @comments
 *
 * @create    20 Jan 2017
 *
 * @author    gmirek
 *
 * @end
 *
 **********************************************************************/
#ifndef INCLUDE_SAI_UT_H
#define INCLUDE_SAI_UT_H

typedef enum
{
   SAI_TEST_BASIC,
   SAI_TEST_AUTOMATED,
   SAI_TEST_CONSOLE,
   SAI_TEST_LOGGING,
   SAI_TEST_CURSES,                    /* This option is not supported. */
   SAI_TEST_MAX
} saiTestType_t;

typedef enum
{
   SAI_TEST_ID_ALL = 0,
   SAI_TEST_ID_MIN = 1,
   SAI_TEST_ID_PDE = SAI_TEST_ID_MIN,
   SAI_TEST_ID_MAX
} saiTestId_t;

extern const char *saiUnitTestModuleNameGet(saiTestId_t testId);
extern int saiUnitTestsRun(saiTestType_t testType, int testRunQA, saiTestId_t testId);

#endif /* INCLUDE_SAI_UT_H */
