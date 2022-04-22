/*
 * Copyright (c) 2011-2019 The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This file was originally distributed by Qualcomm Atheros, Inc.
 * under proprietary terms before Copyright ownership was assigned
 * to the Linux Foundation.
 */

/** ------------------------------------------------------------------------- *
    ------------------------------------------------------------------------- *


    \file csrUtil.c

    Implementation supporting routines for CSR.
========================================================================== */


#include "aniGlobal.h"

#include "palApi.h"
#include "csrSupport.h"
#include "csrInsideApi.h"
#include "smsDebug.h"
#include "smeQosInternal.h"
#include "wlan_qct_wda.h"
#include "vos_utils.h"
#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH
#include "limApi.h"
#endif
#if defined(FEATURE_WLAN_ESE) && !defined(FEATURE_WLAN_ESE_UPLOAD)
#include "csrEse.h"
#endif /* FEATURE_WLAN_ESE && !FEATURE_WLAN_ESE_UPLOAD*/

tANI_U8 csrWpaOui[][ CSR_WPA_OUI_SIZE ] = {
    { 0x00, 0x50, 0xf2, 0x00 },
    { 0x00, 0x50, 0xf2, 0x01 },
    { 0x00, 0x50, 0xf2, 0x02 },
    { 0x00, 0x50, 0xf2, 0x03 },
    { 0x00, 0x50, 0xf2, 0x04 },
    { 0x00, 0x50, 0xf2, 0x05 },
#ifdef FEATURE_WLAN_ESE
    { 0x00, 0x40, 0x96, 0x00 }, // CCKM
#endif /* FEATURE_WLAN_ESE */
};

tANI_U8 csrRSNOui[][ CSR_RSN_OUI_SIZE ] = {
    { 0x00, 0x0F, 0xAC, 0x00 }, // group cipher
    { 0x00, 0x0F, 0xAC, 0x01 }, // WEP-40 or RSN
    { 0x00, 0x0F, 0xAC, 0x02 }, // TKIP or RSN-PSK
    { 0x00, 0x0F, 0xAC, 0x03 }, // Reserved
    { 0x00, 0x0F, 0xAC, 0x04 }, // AES-CCMP
    { 0x00, 0x0F, 0xAC, 0x05 }, // WEP-104
    { 0x00, 0x40, 0x96, 0x00 }, // CCKM
    { 0x00, 0x0F, 0xAC, 0x06 },  // BIP (encryption type) or RSN-PSK-SHA256 (authentication type)
    /* RSN-8021X-SHA256 (authentication type) */
    { 0x00, 0x0F, 0xAC, 0x05 },
#ifdef WLAN_FEATURE_FILS_SK
#define ENUM_FILS_SHA256 9
    /* FILS SHA256 */
    {0x00, 0x0F, 0xAC, 0x0E},
#define ENUM_FILS_SHA384 10
    /* FILS SHA384 */
    {0x00, 0x0F, 0xAC, 0x0F},
#define ENUM_FT_FILS_SHA256 11
    /* FILS FT SHA256 */
    {0x00, 0x0F, 0xAC, 0x10},
#define ENUM_FT_FILS_SHA384 12
    /* FILS FT SHA384 */
    {0x00, 0x0F, 0xAC, 0x11},
#else
    {0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00},
#endif
#ifdef WLAN_FEATURE_SAE
#define ENUM_SAE 13
    /* SAE */
    {0x00, 0x0F, 0xAC, 0x08},
#define ENUM_FT_SAE 14
    /* FT SAE */
    {0x00, 0x0F, 0xAC, 0x09},
#else
    {0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00},
#endif
#define ENUM_OWE 15
    /* OWE https://tools.ietf.org/html/rfc8110 */
    {0x00, 0x0F, 0xAC, 0x12},
    /* define new oui here */
};

#ifdef FEATURE_WLAN_WAPI
tANI_U8 csrWapiOui[][ CSR_WAPI_OUI_SIZE ] = {
    { 0x00, 0x14, 0x72, 0x00 }, // Reserved
    { 0x00, 0x14, 0x72, 0x01 }, // WAI certificate or SMS4
    { 0x00, 0x14, 0x72, 0x02 } // WAI PSK
};
#endif /* FEATURE_WLAN_WAPI */
tANI_U8 csrWmeInfoOui[ CSR_WME_OUI_SIZE ] = { 0x00, 0x50, 0xf2, 0x02 };
tANI_U8 csrWmeParmOui[ CSR_WME_OUI_SIZE ] = { 0x00, 0x50, 0xf2, 0x02 };

static tCsrIELenInfo gCsrIELengthTable[] = {
/* 000 */ { SIR_MAC_SSID_EID_MIN, SIR_MAC_SSID_EID_MAX },
/* 001 */ { SIR_MAC_RATESET_EID_MIN, SIR_MAC_RATESET_EID_MAX },
/* 002 */ { SIR_MAC_FH_PARAM_SET_EID_MIN, SIR_MAC_FH_PARAM_SET_EID_MAX },
/* 003 */ { SIR_MAC_DS_PARAM_SET_EID_MIN, SIR_MAC_DS_PARAM_SET_EID_MAX },
/* 004 */ { SIR_MAC_CF_PARAM_SET_EID_MIN, SIR_MAC_CF_PARAM_SET_EID_MAX },
/* 005 */ { SIR_MAC_TIM_EID_MIN, SIR_MAC_TIM_EID_MAX },
/* 006 */ { SIR_MAC_IBSS_PARAM_SET_EID_MIN, SIR_MAC_IBSS_PARAM_SET_EID_MAX },
/* 007 */ { SIR_MAC_COUNTRY_EID_MIN, SIR_MAC_COUNTRY_EID_MAX },
/* 008 */ { SIR_MAC_FH_PARAMS_EID_MIN, SIR_MAC_FH_PARAMS_EID_MAX },
/* 009 */ { SIR_MAC_FH_PATTERN_EID_MIN, SIR_MAC_FH_PATTERN_EID_MAX },
/* 010 */ { SIR_MAC_REQUEST_EID_MIN, SIR_MAC_REQUEST_EID_MAX },
/* 011 */ { SIR_MAC_QBSS_LOAD_EID_MIN, SIR_MAC_QBSS_LOAD_EID_MAX },
/* 012 */ { SIR_MAC_EDCA_PARAM_SET_EID_MIN, SIR_MAC_EDCA_PARAM_SET_EID_MAX },
/* 013 */ { SIR_MAC_TSPEC_EID_MIN, SIR_MAC_TSPEC_EID_MAX },
/* 014 */ { SIR_MAC_TCLAS_EID_MIN, SIR_MAC_TCLAS_EID_MAX },
/* 015 */ { SIR_MAC_QOS_SCHEDULE_EID_MIN, SIR_MAC_QOS_SCHEDULE_EID_MAX },
/* 016 */ { SIR_MAC_CHALLENGE_TEXT_EID_MIN, SIR_MAC_CHALLENGE_TEXT_EID_MAX },
/* 017 */ { 0, 255 },
/* 018 */ { 0, 255 },
/* 019 */ { 0, 255 },
/* 020 */ { 0, 255 },
/* 021 */ { 0, 255 },
/* 022 */ { 0, 255 },
/* 023 */ { 0, 255 },
/* 024 */ { 0, 255 },
/* 025 */ { 0, 255 },
/* 026 */ { 0, 255 },
/* 027 */ { 0, 255 },
/* 028 */ { 0, 255 },
/* 029 */ { 0, 255 },
/* 030 */ { 0, 255 },
/* 031 */ { 0, 255 },
/* 032 */ { SIR_MAC_PWR_CONSTRAINT_EID_MIN, SIR_MAC_PWR_CONSTRAINT_EID_MAX },
/* 033 */ { SIR_MAC_PWR_CAPABILITY_EID_MIN, SIR_MAC_PWR_CAPABILITY_EID_MAX },
/* 034 */ { SIR_MAC_TPC_REQ_EID_MIN, SIR_MAC_TPC_REQ_EID_MAX },
/* 035 */ { SIR_MAC_TPC_RPT_EID_MIN, SIR_MAC_TPC_RPT_EID_MAX },
/* 036 */ { SIR_MAC_SPRTD_CHNLS_EID_MIN, SIR_MAC_SPRTD_CHNLS_EID_MAX },
/* 037 */ { SIR_MAC_CHNL_SWITCH_ANN_EID_MIN, SIR_MAC_CHNL_SWITCH_ANN_EID_MAX },
/* 038 */ { SIR_MAC_MEAS_REQ_EID_MIN, SIR_MAC_MEAS_REQ_EID_MAX },
/* 039 */ { SIR_MAC_MEAS_RPT_EID_MIN, SIR_MAC_MEAS_RPT_EID_MAX },
/* 040 */ { SIR_MAC_QUIET_EID_MIN, SIR_MAC_QUIET_EID_MAX },
/* 041 */ { SIR_MAC_IBSS_DFS_EID_MIN, SIR_MAC_IBSS_DFS_EID_MAX },
/* 042 */ { SIR_MAC_ERP_INFO_EID_MIN, SIR_MAC_ERP_INFO_EID_MAX },
/* 043 */ { SIR_MAC_TS_DELAY_EID_MIN, SIR_MAC_TS_DELAY_EID_MAX },
/* 044 */ { SIR_MAC_TCLAS_PROC_EID_MIN, SIR_MAC_TCLAS_PROC_EID_MAX },
/* 045 */ { SIR_MAC_QOS_ACTION_EID_MIN, SIR_MAC_QOS_ACTION_EID_MAX },
/* 046 */ { SIR_MAC_QOS_CAPABILITY_EID_MIN, SIR_MAC_QOS_CAPABILITY_EID_MAX },
/* 047 */ { 0, 255 },
/* 048 */ { SIR_MAC_RSN_EID_MIN, SIR_MAC_RSN_EID_MAX },
/* 049 */ { 0, 255 },
/* 050 */ { SIR_MAC_EXTENDED_RATE_EID_MIN, SIR_MAC_EXTENDED_RATE_EID_MAX },
/* 051 */ { 0, 255 },
/* 052 */ { 0, 255 },
/* 053 */ { 0, 255 },
/* 054 */ { 0, 255 },
/* 055 */ { 0, 255 },
/* 056 */ { 0, 255 },
/* 057 */ { 0, 255 },
/* 058 */ { 0, 255 },
/* 059 */ { SIR_MAC_OPERATING_CLASS_EID_MIN, SIR_MAC_OPERATING_CLASS_EID_MAX },
/* 060 */ { SIR_MAC_CHNL_EXTENDED_SWITCH_ANN_EID_MIN, SIR_MAC_CHNL_EXTENDED_SWITCH_ANN_EID_MAX},
/* 061 */ { 0, 255 },
/* 062 */ { 0, 255 },
/* 063 */ { 0, 255 },
/* 064 */ { 0, 255 },
/* 065 */ { 0, 255 },
/* 066 */ { 0, 255 },
/* 067 */ { 0, 255 },
#ifdef FEATURE_WLAN_WAPI
/* 068 */ { DOT11F_EID_WAPI, DOT11F_IE_WAPI_MAX_LEN },
#else
/* 068 */ { 0, 255 },
#endif /* FEATURE_WLAN_WAPI */
/* 069 */ { 0, 255 },
/* 070 */ { 0, 255 },
/* 071 */ { 0, 255 },
/* 072 */ { 0, 255 },
/* 073 */ { 0, 255 },
/* 074 */ { 0, 255 },
/* 075 */ { 0, 255 },
/* 076 */ { 0, 255 },
/* 077 */ { 0, 255 },
/* 078 */ { 0, 255 },
/* 079 */ { 0, 255 },
/* 080 */ { 0, 255 },
/* 081 */ { 0, 255 },
/* 082 */ { 0, 255 },
/* 083 */ { 0, 255 },
/* 084 */ { 0, 255 },
/* 085 */ { 0, 255 },
/* 086 */ { 0, 255 },
/* 087 */ { 0, 255 },
/* 088 */ { 0, 255 },
/* 089 */ { 0, 255 },
/* 090 */ { 0, 255 },
/* 091 */ { 0, 255 },
/* 092 */ { 0, 255 },
/* 093 */ { 0, 255 },
/* 094 */ { 0, 255 },
/* 095 */ { 0, 255 },
/* 096 */ { 0, 255 },
/* 097 */ { 0, 255 },
/* 098 */ { 0, 255 },
/* 099 */ { 0, 255 },
/* 100 */ { 0, 255 },
/* 101 */ { 0, 255 },
/* 102 */ { 0, 255 },
/* 103 */ { 0, 255 },
/* 104 */ { 0, 255 },
/* 105 */ { 0, 255 },
/* 106 */ { 0, 255 },
/* 107 */ { 0, 255 },
/* 108 */ { 0, 255 },
/* 109 */ { 0, 255 },
/* 110 */ { 0, 255 },
/* 111 */ { 0, 255 },
/* 112 */ { 0, 255 },
/* 113 */ { 0, 255 },
/* 114 */ { 0, 255 },
/* 115 */ { 0, 255 },
/* 116 */ { 0, 255 },
/* 117 */ { 0, 255 },
/* 118 */ { 0, 255 },
/* 119 */ { 0, 255 },
/* 120 */ { 0, 255 },
/* 121 */ { 0, 255 },
/* 122 */ { 0, 255 },
/* 123 */ { 0, 255 },
/* 124 */ { 0, 255 },
/* 125 */ { 0, 255 },
/* 126 */ { 0, 255 },
/* 127 */ { 0, 255 },
/* 128 */ { 0, 255 },
/* 129 */ { 0, 255 },
/* 130 */ { 0, 255 },
/* 131 */ { 0, 255 },
/* 132 */ { 0, 255 },
/* 133 */ { 0, 255 },
/* 134 */ { 0, 255 },
/* 135 */ { 0, 255 },
/* 136 */ { 0, 255 },
/* 137 */ { 0, 255 },
/* 138 */ { 0, 255 },
/* 139 */ { 0, 255 },
/* 140 */ { 0, 255 },
/* 141 */ { 0, 255 },
/* 142 */ { 0, 255 },
/* 143 */ { 0, 255 },
/* 144 */ { 0, 255 },
/* 145 */ { 0, 255 },
/* 146 */ { 0, 255 },
/* 147 */ { 0, 255 },
/* 148 */ { 0, 255 },
/* 149 */ { 0, 255 },
/* 150 */ { 0, 255 },
/* 151 */ { 0, 255 },
/* 152 */ { 0, 255 },
/* 153 */ { 0, 255 },
/* 154 */ { 0, 255 },
/* 155 */ { 0, 255 },
/* 156 */ { 0, 255 },
/* 157 */ { 0, 255 },
/* 158 */ { 0, 255 },
/* 159 */ { 0, 255 },
/* 160 */ { 0, 255 },
/* 161 */ { 0, 255 },
/* 162 */ { 0, 255 },
/* 163 */ { 0, 255 },
/* 164 */ { 0, 255 },
/* 165 */ { 0, 255 },
/* 166 */ { 0, 255 },
/* 167 */ { 0, 255 },
/* 168 */ { 0, 255 },
/* 169 */ { 0, 255 },
/* 170 */ { 0, 255 },
/* 171 */ { 0, 255 },
/* 172 */ { 0, 255 },
/* 173 */ { 0, 255 },
/* 174 */ { 0, 255 },
/* 175 */ { 0, 255 },
/* 176 */ { 0, 255 },
/* 177 */ { 0, 255 },
/* 178 */ { 0, 255 },
/* 179 */ { 0, 255 },
/* 180 */ { 0, 255 },
/* 181 */ { 0, 255 },
/* 182 */ { 0, 255 },
/* 183 */ { 0, 255 },
/* 184 */ { 0, 255 },
/* 185 */ { 0, 255 },
/* 186 */ { 0, 255 },
/* 187 */ { 0, 255 },
/* 188 */ { 0, 255 },
/* 189 */ { 0, 255 },
/* 190 */ { 0, 255 },
/* 191 */ { 0, 255 },
/* 192 */ { 0, 255 },
/* 193 */ { 0, 255 },
/* 194 */ { 0, 255 },
/* 195 */ { 0, 255 },
/* 196 */ { 0, 255 },
/* 197 */ { 0, 255 },
/* 198 */ { 0, 255 },
/* 199 */ { 0, 255 },
/* 200 */ { 0, 255 },
/* 201 */ { 0, 255 },
/* 202 */ { 0, 255 },
/* 203 */ { 0, 255 },
/* 204 */ { 0, 255 },
/* 205 */ { 0, 255 },
/* 206 */ { 0, 255 },
/* 207 */ { 0, 255 },
/* 208 */ { 0, 255 },
/* 209 */ { 0, 255 },
/* 210 */ { 0, 255 },
/* 211 */ { 0, 255 },
/* 212 */ { 0, 255 },
/* 213 */ { 0, 255 },
/* 214 */ { 0, 255 },
/* 215 */ { 0, 255 },
/* 216 */ { 0, 255 },
/* 217 */ { 0, 255 },
/* 218 */ { 0, 255 },
/* 219 */ { 0, 255 },
/* 220 */ { 0, 255 },
/* 221 */ { SIR_MAC_WPA_EID_MIN, SIR_MAC_WPA_EID_MAX },
/* 222 */ { 0, 255 },
/* 223 */ { 0, 255 },
/* 224 */ { 0, 255 },
/* 225 */ { 0, 255 },
/* 226 */ { 0, 255 },
/* 227 */ { 0, 255 },
/* 228 */ { 0, 255 },
/* 229 */ { 0, 255 },
/* 230 */ { 0, 255 },
/* 231 */ { 0, 255 },
/* 232 */ { 0, 255 },
/* 233 */ { 0, 255 },
/* 234 */ { 0, 255 },
/* 235 */ { 0, 255 },
/* 236 */ { 0, 255 },
/* 237 */ { 0, 255 },
/* 238 */ { 0, 255 },
/* 239 */ { 0, 255 },
/* 240 */ { 0, 255 },
/* 241 */ { 0, 255 },
/* 242 */ { 0, 255 },
/* 243 */ { 0, 255 },
/* 244 */ { 0, 255 },
/* 245 */ { 0, 255 },
/* 246 */ { 0, 255 },
/* 247 */ { 0, 255 },
/* 248 */ { 0, 255 },
/* 249 */ { 0, 255 },
/* 250 */ { 0, 255 },
/* 251 */ { 0, 255 },
/* 252 */ { 0, 255 },
/* 253 */ { 0, 255 },
/* 254 */ { 0, 255 },
/* 255 */ { SIR_MAC_ANI_WORKAROUND_EID_MIN, SIR_MAC_ANI_WORKAROUND_EID_MAX }
};

extern const tRfChannelProps rfChannels[NUM_RF_CHANNELS];

////////////////////////////////////////////////////////////////////////

/**
 * \var gPhyRatesSuppt
 *
 * \brief Rate support lookup table
 *
 *
 * This is a  lookup table indexing rates &  configuration parameters to
 * support.  Given a rate (in  unites of 0.5Mpbs) & three booleans (MIMO
 * Enabled, Channel  Bonding Enabled, & Concatenation  Enabled), one can
 * determine  whether  the given  rate  is  supported  by computing  two
 * indices.  The  first maps  the rate to  table row as  indicated below
 * (i.e. eHddSuppRate_6Mbps maps to  row zero, eHddSuppRate_9Mbps to row
 * 1, and so on).  Index two can be computed like so:
 *
 * \code
   idx2 = ( fEsf  ? 0x4 : 0x0 ) |
          ( fCb   ? 0x2 : 0x0 ) |
          ( fMimo ? 0x1 : 0x0 );
 * \endcode
 *
 *
 * Given that:
 *
 \code
   fSupported = gPhyRatesSuppt[idx1][idx2];
 \endcode
 *
 *
 * This table is based on  the document "PHY Supported Rates.doc".  This
 * table is  permissive in that a  rate is reflected  as being supported
 * even  when turning  off an  enabled feature  would be  required.  For
 * instance, "PHY Supported Rates"  lists 42Mpbs as unsupported when CB,
 * ESF, &  MIMO are all  on.  However,  if we turn  off either of  CB or
 * MIMO, it then becomes supported.   Therefore, we mark it as supported
 * even in index 7 of this table.
 *
 *
 */

static const tANI_BOOLEAN gPhyRatesSuppt[24][8] = {

    // SSF   SSF    SSF    SSF    ESF    ESF    ESF    ESF
    // SIMO  MIMO   SIMO   MIMO   SIMO   MIMO   SIMO   MIMO
    // No CB No CB  CB     CB     No CB  No CB  CB     CB
    { TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE  }, // 6Mbps
    { TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE  }, // 9Mbps
    { TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE  }, // 12Mbps
    { TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE  }, // 18Mbps
    { FALSE, FALSE, TRUE,  TRUE,  FALSE, FALSE, TRUE,  TRUE  }, // 20Mbps
    { TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE  }, // 24Mbps
    { TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE  }, // 36Mbps
    { FALSE, FALSE, TRUE,  TRUE,  FALSE, TRUE,  TRUE,  TRUE  }, // 40Mbps
    { FALSE, FALSE, TRUE,  TRUE,  FALSE, TRUE,  TRUE,  TRUE  }, // 42Mbps
    { TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE  }, // 48Mbps
    { TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE  }, // 54Mbps
    { FALSE, TRUE,  TRUE,  TRUE,  FALSE, TRUE,  TRUE,  TRUE  }, // 72Mbps
    { FALSE, FALSE, TRUE,  TRUE,  FALSE, TRUE,  TRUE,  TRUE  }, // 80Mbps
    { FALSE, FALSE, TRUE,  TRUE,  FALSE, TRUE,  TRUE,  TRUE  }, // 84Mbps
    { FALSE, TRUE,  TRUE,  TRUE,  FALSE, TRUE,  TRUE,  TRUE  }, // 96Mbps
    { FALSE, TRUE,  TRUE,  TRUE,  FALSE, TRUE,  TRUE,  TRUE  }, // 108Mbps
    { FALSE, FALSE, TRUE,  TRUE,  FALSE, TRUE,  TRUE,  TRUE  }, // 120Mbps
    { FALSE, FALSE, TRUE,  TRUE,  FALSE, TRUE,  TRUE,  TRUE  }, // 126Mbps
    { FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, TRUE  }, // 144Mbps
    { FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, TRUE  }, // 160Mbps
    { FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, TRUE  }, // 168Mbps
    { FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, TRUE  }, // 192Mbps
    { FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, TRUE  }, // 216Mbps
    { FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, TRUE  }, // 240Mbps

};

#define CASE_RETURN_STR(n) case (n): return (#n)

const char *
get_eRoamCmdStatus_str(eRoamCmdStatus val)
{
    switch (val)
    {
        CASE_RETURN_STR(eCSR_ROAM_CANCELLED);
        CASE_RETURN_STR(eCSR_ROAM_FAILED);
        CASE_RETURN_STR(eCSR_ROAM_ROAMING_START);
        CASE_RETURN_STR(eCSR_ROAM_ROAMING_COMPLETION);
        CASE_RETURN_STR(eCSR_ROAM_CONNECT_COMPLETION);
        CASE_RETURN_STR(eCSR_ROAM_ASSOCIATION_START);
        CASE_RETURN_STR(eCSR_ROAM_ASSOCIATION_COMPLETION);
        CASE_RETURN_STR(eCSR_ROAM_DISASSOCIATED);
        CASE_RETURN_STR(eCSR_ROAM_SHOULD_ROAM);
        CASE_RETURN_STR(eCSR_ROAM_SCAN_FOUND_NEW_BSS);
        CASE_RETURN_STR(eCSR_ROAM_LOSTLINK);
        CASE_RETURN_STR(eCSR_ROAM_LOSTLINK_DETECTED);
        CASE_RETURN_STR(eCSR_ROAM_MIC_ERROR_IND);
        CASE_RETURN_STR(eCSR_ROAM_IBSS_IND);
        CASE_RETURN_STR(eCSR_ROAM_CONNECT_STATUS_UPDATE);
        CASE_RETURN_STR(eCSR_ROAM_GEN_INFO);
        CASE_RETURN_STR(eCSR_ROAM_SET_KEY_COMPLETE);
        CASE_RETURN_STR(eCSR_ROAM_REMOVE_KEY_COMPLETE);
        CASE_RETURN_STR(eCSR_ROAM_IBSS_LEAVE);
        CASE_RETURN_STR(eCSR_ROAM_WDS_IND);
        CASE_RETURN_STR(eCSR_ROAM_INFRA_IND);
        CASE_RETURN_STR(eCSR_ROAM_WPS_PBC_PROBE_REQ_IND);
#ifdef WLAN_FEATURE_VOWIFI_11R
        CASE_RETURN_STR(eCSR_ROAM_FT_RESPONSE);
#endif
        CASE_RETURN_STR(eCSR_ROAM_FT_START);
        CASE_RETURN_STR(eCSR_ROAM_REMAIN_CHAN_READY);
        CASE_RETURN_STR(eCSR_ROAM_SESSION_OPENED);
        CASE_RETURN_STR(eCSR_ROAM_FT_REASSOC_FAILED);
#ifdef FEATURE_WLAN_LFR
        CASE_RETURN_STR(eCSR_ROAM_PMK_NOTIFY);
#endif
#ifdef FEATURE_WLAN_LFR_METRICS
        CASE_RETURN_STR(eCSR_ROAM_PREAUTH_INIT_NOTIFY);
        CASE_RETURN_STR(eCSR_ROAM_PREAUTH_STATUS_SUCCESS);
        CASE_RETURN_STR(eCSR_ROAM_PREAUTH_STATUS_FAILURE);
        CASE_RETURN_STR(eCSR_ROAM_HANDOVER_SUCCESS);
#endif
#ifdef FEATURE_WLAN_TDLS
        CASE_RETURN_STR(eCSR_ROAM_TDLS_STATUS_UPDATE);
        CASE_RETURN_STR(eCSR_ROAM_RESULT_MGMT_TX_COMPLETE_IND);
#endif
        CASE_RETURN_STR(eCSR_ROAM_DISCONNECT_ALL_P2P_CLIENTS);
        CASE_RETURN_STR(eCSR_ROAM_SEND_P2P_STOP_BSS);
#ifdef WLAN_FEATURE_11W
        CASE_RETURN_STR(eCSR_ROAM_UNPROT_MGMT_FRAME_IND);
#endif
#ifdef WLAN_FEATURE_RMC
        CASE_RETURN_STR(eCSR_ROAM_IBSS_PEER_INFO_COMPLETE);
#endif
#ifdef WLAN_FEATURE_AP_HT40_24G
        CASE_RETURN_STR(eCSR_ROAM_2040_COEX_INFO_IND);
#endif
#if defined(FEATURE_WLAN_ESE) && defined(FEATURE_WLAN_ESE_UPLOAD)
        CASE_RETURN_STR(eCSR_ROAM_TSM_IE_IND);
        CASE_RETURN_STR(eCSR_ROAM_CCKM_PREAUTH_NOTIFY);
        CASE_RETURN_STR(eCSR_ROAM_ESE_ADJ_AP_REPORT_IND);
        CASE_RETURN_STR(eCSR_ROAM_ESE_BCN_REPORT_IND);
#endif /* FEATURE_WLAN_ESE && FEATURE_WLAN_ESE_UPLOAD */
        CASE_RETURN_STR(eCSR_ROAM_SAE_COMPUTE);
    default:
        return "unknown";
    }
}

const char *
get_eCsrRoamResult_str(eCsrRoamResult val)
{
    switch (val)
    {
        CASE_RETURN_STR(eCSR_ROAM_RESULT_NONE);
        CASE_RETURN_STR(eCSR_ROAM_RESULT_FAILURE);
        CASE_RETURN_STR(eCSR_ROAM_RESULT_ASSOCIATED);
        CASE_RETURN_STR(eCSR_ROAM_RESULT_NOT_ASSOCIATED);
        CASE_RETURN_STR(eCSR_ROAM_RESULT_MIC_FAILURE);
        CASE_RETURN_STR(eCSR_ROAM_RESULT_FORCED);
        CASE_RETURN_STR(eCSR_ROAM_RESULT_DISASSOC_IND);
        CASE_RETURN_STR(eCSR_ROAM_RESULT_DEAUTH_IND);
        CASE_RETURN_STR(eCSR_ROAM_RESULT_CAP_CHANGED);
        CASE_RETURN_STR(eCSR_ROAM_RESULT_IBSS_CONNECT);
        CASE_RETURN_STR(eCSR_ROAM_RESULT_IBSS_INACTIVE);
        CASE_RETURN_STR(eCSR_ROAM_RESULT_IBSS_NEW_PEER);
        CASE_RETURN_STR(eCSR_ROAM_RESULT_IBSS_COALESCED);
    default:
        return "unknown";
    }
}



tANI_BOOLEAN csrGetBssIdBssDesc( tHalHandle hHal, tSirBssDescription *pSirBssDesc, tCsrBssid *pBssId )
{
    vos_mem_copy(pBssId, &pSirBssDesc->bssId[ 0 ], sizeof(tCsrBssid));
    return( TRUE );
}


tANI_BOOLEAN csrIsBssIdEqual( tHalHandle hHal, tSirBssDescription *pSirBssDesc1, tSirBssDescription *pSirBssDesc2 )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tANI_BOOLEAN fEqual = FALSE;
    tCsrBssid bssId1;
    tCsrBssid bssId2;

    do {
        if ( !pSirBssDesc1 ) break;
        if ( !pSirBssDesc2 ) break;

        if ( !csrGetBssIdBssDesc( pMac, pSirBssDesc1, &bssId1 ) ) break;
        if ( !csrGetBssIdBssDesc( pMac, pSirBssDesc2, &bssId2 ) ) break;

        //sirCompareMacAddr
        fEqual = csrIsMacAddressEqual(pMac, &bssId1, &bssId2);

    } while( 0 );

    return( fEqual );
}

tANI_BOOLEAN csrIsConnStateConnectedIbss( tpAniSirGlobal pMac, tANI_U32 sessionId )
{
    return( eCSR_ASSOC_STATE_TYPE_IBSS_CONNECTED == pMac->roam.roamSession[sessionId].connectState );
}

tANI_BOOLEAN csrIsConnStateDisconnectedIbss( tpAniSirGlobal pMac, tANI_U32 sessionId )
{
    return( eCSR_ASSOC_STATE_TYPE_IBSS_DISCONNECTED == pMac->roam.roamSession[sessionId].connectState );
}

tANI_BOOLEAN csrIsConnStateConnectedInfra( tpAniSirGlobal pMac, tANI_U32 sessionId )
{
    return( eCSR_ASSOC_STATE_TYPE_INFRA_ASSOCIATED == pMac->roam.roamSession[sessionId].connectState );
}

tANI_BOOLEAN csrIsConnStateConnected( tpAniSirGlobal pMac, tANI_U32 sessionId )
{
    if( csrIsConnStateConnectedIbss( pMac, sessionId ) || csrIsConnStateConnectedInfra( pMac, sessionId ) || csrIsConnStateConnectedWds( pMac, sessionId) )
        return TRUE;
    else
        return FALSE;
}

tANI_BOOLEAN csrIsConnStateInfra( tpAniSirGlobal pMac, tANI_U32 sessionId )
{
    return( csrIsConnStateConnectedInfra( pMac, sessionId ) );
}

tANI_BOOLEAN csrIsConnStateIbss( tpAniSirGlobal pMac, tANI_U32 sessionId )
{
    return( csrIsConnStateConnectedIbss( pMac, sessionId ) || csrIsConnStateDisconnectedIbss( pMac, sessionId ) );
}


tANI_BOOLEAN csrIsConnStateConnectedWds( tpAniSirGlobal pMac, tANI_U32 sessionId )
{
    return( eCSR_ASSOC_STATE_TYPE_WDS_CONNECTED == pMac->roam.roamSession[sessionId].connectState );
}

tANI_BOOLEAN csrIsConnStateConnectedInfraAp( tpAniSirGlobal pMac, tANI_U32 sessionId )
{
    return( (eCSR_ASSOC_STATE_TYPE_INFRA_CONNECTED == pMac->roam.roamSession[sessionId].connectState) ||
        (eCSR_ASSOC_STATE_TYPE_INFRA_DISCONNECTED == pMac->roam.roamSession[sessionId].connectState ) );
}

tANI_BOOLEAN csrIsConnStateDisconnectedWds( tpAniSirGlobal pMac, tANI_U32 sessionId )
{
    return( eCSR_ASSOC_STATE_TYPE_WDS_DISCONNECTED == pMac->roam.roamSession[sessionId].connectState );
}

tANI_BOOLEAN csrIsConnStateWds( tpAniSirGlobal pMac, tANI_U32 sessionId )
{
    return( csrIsConnStateConnectedWds( pMac, sessionId ) ||
        csrIsConnStateDisconnectedWds( pMac, sessionId ) );
}

tANI_BOOLEAN csrIsConnStateAp( tpAniSirGlobal pMac,  tANI_U32 sessionId )
{
    tCsrRoamSession *pSession;
    pSession = CSR_GET_SESSION(pMac, sessionId);
    if (!pSession)
        return eANI_BOOLEAN_FALSE;
    if ( CSR_IS_INFRA_AP(&pSession->connectedProfile) )
    {
        return eANI_BOOLEAN_TRUE;
    }
    return eANI_BOOLEAN_FALSE;
}

tANI_BOOLEAN csrIsAnySessionInConnectState( tpAniSirGlobal pMac )
{
    tANI_U32 i;
    tANI_BOOLEAN fRc = eANI_BOOLEAN_FALSE;

    for( i = 0; i < CSR_ROAM_SESSION_MAX; i++ )
    {
        if( CSR_IS_SESSION_VALID( pMac, i ) &&
            ( csrIsConnStateInfra( pMac, i )
            || csrIsConnStateIbss( pMac, i )
            || csrIsConnStateAp( pMac, i) ) )
        {
            fRc = eANI_BOOLEAN_TRUE;
            break;
        }
    }

    return ( fRc );
}

/**
 * csr_is_ndi_started() - function to check if NDI is started
 * @mac_ctx: handle to mac context
 * @session_id: session identifier
 *
 * returns: true if NDI is started, false otherwise
 */
bool csr_is_ndi_started(tpAniSirGlobal mac_ctx, uint32_t session_id)
{
	tCsrRoamSession *session = CSR_GET_SESSION(mac_ctx, session_id);

	if (!session)
		return false;

	return (eCSR_CONNECT_STATE_TYPE_NDI_STARTED == session->connectState);
}

tANI_S8 csrGetInfraSessionId( tpAniSirGlobal pMac )
{
    tANI_U8 i;
    tANI_S8 sessionid = -1;

    for( i = 0; i < CSR_ROAM_SESSION_MAX; i++ )
    {
        if( CSR_IS_SESSION_VALID( pMac, i ) && csrIsConnStateInfra( pMac, i )  )
        {
            sessionid = i;
            break;
        }
    }

    return ( sessionid );
}

tANI_U8 csrGetInfraOperationChannel( tpAniSirGlobal pMac, tANI_U8 sessionId)
{
    tANI_U8 channel;

    if( CSR_IS_SESSION_VALID( pMac, sessionId ))
    {
        channel = pMac->roam.roamSession[sessionId].connectedProfile.operationChannel;
    }
    else
    {
        channel = 0;
    }
    return channel;
}

tANI_BOOLEAN csrIsSessionClientAndConnected(tpAniSirGlobal pMac, tANI_U8 sessionId)
{
    tCsrRoamSession *pSession = NULL;
    if (CSR_IS_SESSION_VALID( pMac, sessionId) && csrIsConnStateInfra( pMac, sessionId))
    {
        pSession = CSR_GET_SESSION( pMac, sessionId);
        if (NULL != pSession->pCurRoamProfile)
        {
            if ((pSession->pCurRoamProfile->csrPersona == VOS_STA_MODE) ||
                (pSession->pCurRoamProfile->csrPersona == VOS_P2P_CLIENT_MODE))
                return TRUE;
        }
    }
    return FALSE;
}
//This routine will return operating channel on FIRST BSS that is active/operating to be used for concurrency mode.
//If other BSS is not up or not connected it will return 0

tANI_U8 csrGetConcurrentOperationChannel( tpAniSirGlobal pMac )
{
  tCsrRoamSession *pSession = NULL;
  tANI_U8 i = 0;

  for( i = 0; i < CSR_ROAM_SESSION_MAX; i++ )
  {
      if( CSR_IS_SESSION_VALID( pMac, i ) )
      {
          pSession = CSR_GET_SESSION( pMac, i );

          if (NULL != pSession->pCurRoamProfile)
          {
              if (
                      (((pSession->pCurRoamProfile->csrPersona == VOS_STA_MODE) ||
                        (pSession->pCurRoamProfile->csrPersona == VOS_P2P_CLIENT_MODE)) &&
                       (pSession->connectState == eCSR_ASSOC_STATE_TYPE_INFRA_ASSOCIATED))
                      ||
                      (((pSession->pCurRoamProfile->csrPersona == VOS_P2P_GO_MODE) ||
                        (pSession->pCurRoamProfile->csrPersona == VOS_STA_SAP_MODE)) &&
                       (pSession->connectState != eCSR_ASSOC_STATE_TYPE_NOT_CONNECTED))
                 )
                  return (pSession->connectedProfile.operationChannel);
          }

      }
  }
  return 0;
}
#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH

#define HALF_BW_OF(eCSR_bw_val) ((eCSR_bw_val)/2)

/* calculation of center channel based on V/HT BW and WIFI channel bw=5MHz) */

#define CSR_GET_HT40_PLUS_CCH(och) ((och)+2)
#define CSR_GET_HT40_MINUS_CCH(och) ((och)-2)

#define CSR_GET_HT80_PLUS_LL_CCH(och) ((och)+6)
#define CSR_GET_HT80_PLUS_HL_CCH(och) ((och)+2)
#define CSR_GET_HT80_MINUS_LH_CCH(och) ((och)-2)
#define CSR_GET_HT80_MINUS_HH_CCH(och) ((och)-6)

void csrGetChFromHTProfile (tpAniSirGlobal pMac, tCsrRoamHTProfile *htp,
                            tANI_U16 och, tANI_U16 *cfreq, tANI_U16 *hbw)
{
    tANI_U16 cch, ch_bond;

    if (och > 14)
        ch_bond = pMac->roam.configParam.channelBondingMode5GHz;
    else
        ch_bond = pMac->roam.configParam.channelBondingMode24GHz;

    cch = och;
    *hbw = HALF_BW_OF(eCSR_BW_20MHz_VAL);

    if (!ch_bond) {
        goto ret;
    }
    smsLog(pMac, LOG1,
        FL("##HTC: %d scbw: %d rcbw: %d sco: %d"
#ifdef WLAN_FEATURE_11AC
           "VHTC: %d apc: %d apbw: %d"
#endif
        ),
        htp->htCapability, htp->htSupportedChannelWidthSet,
        htp->htRecommendedTxWidthSet, htp->htSecondaryChannelOffset,
#ifdef WLAN_FEATURE_11AC
        htp->vhtCapability, htp->apCenterChan, htp->apChanWidth
#endif
        );

#ifdef WLAN_FEATURE_11AC
    if (htp->vhtCapability) {
        cch = htp->apCenterChan;
        if (htp->apChanWidth == WNI_CFG_VHT_CHANNEL_WIDTH_80MHZ)
            *hbw = HALF_BW_OF(eCSR_BW_80MHz_VAL);
        else if (htp->apChanWidth == WNI_CFG_VHT_CHANNEL_WIDTH_160MHZ)
            *hbw = HALF_BW_OF(eCSR_BW_160MHz_VAL);

        if (!*hbw && htp->htCapability) {
            if (htp->htSupportedChannelWidthSet == eHT_CHANNEL_WIDTH_40MHZ)
                *hbw = HALF_BW_OF(eCSR_BW_40MHz_VAL);
            else
                *hbw = HALF_BW_OF(eCSR_BW_20MHz_VAL);
        }
     } else
#endif
    if (htp->htCapability) {
        if (htp->htSupportedChannelWidthSet == eHT_CHANNEL_WIDTH_40MHZ) {
            *hbw = HALF_BW_OF(eCSR_BW_40MHz_VAL);
            if (htp->htSecondaryChannelOffset == PHY_DOUBLE_CHANNEL_LOW_PRIMARY)
                cch = CSR_GET_HT40_PLUS_CCH(och);
            else if (htp->htSecondaryChannelOffset ==
                                                PHY_DOUBLE_CHANNEL_HIGH_PRIMARY)
                cch = CSR_GET_HT40_MINUS_CCH(och);
        } else {
            cch = och;
            *hbw = HALF_BW_OF(eCSR_BW_20MHz_VAL);
        }
    }

ret:
    *cfreq = vos_chan_to_freq(cch);
    return;
}

v_U16_t csrCheckConcurrentChannelOverlap(tpAniSirGlobal pMac, v_U16_t sap_ch,
                                eCsrPhyMode sap_phymode, v_U8_t cc_switch_mode)
{
    tCsrRoamSession *pSession = NULL;
    v_U8_t i = 0,  chb = PHY_SINGLE_CHANNEL_CENTERED;
    v_U16_t intf_ch=0, sap_hbw = 0, intf_hbw = 0, intf_cfreq = 0, sap_cfreq = 0;
    v_U16_t sap_lfreq, sap_hfreq, intf_lfreq, intf_hfreq, sap_cch;

    if (pMac->roam.configParam.cc_switch_mode == VOS_MCC_TO_SCC_SWITCH_DISABLE)
        return 0;

    if (sap_ch !=0) {

        sap_cch = sap_ch;
        sap_hbw = HALF_BW_OF(eCSR_BW_20MHz_VAL);

        if (sap_ch > 14)
            chb = pMac->roam.configParam.channelBondingMode5GHz;
        else
            chb = pMac->roam.configParam.channelBondingMode24GHz;

        if (chb) {
            if (sap_phymode == eCSR_DOT11_MODE_11n ||
                sap_phymode == eCSR_DOT11_MODE_11n_ONLY) {

                sap_hbw = HALF_BW_OF(eCSR_BW_40MHz_VAL);
                if (chb == PHY_DOUBLE_CHANNEL_LOW_PRIMARY)
                    sap_cch = CSR_GET_HT40_PLUS_CCH(sap_ch);
                else if (chb == PHY_DOUBLE_CHANNEL_HIGH_PRIMARY)
                    sap_cch = CSR_GET_HT40_MINUS_CCH(sap_ch);

            }
#ifdef WLAN_FEATURE_11AC
            else if (sap_phymode == eCSR_DOT11_MODE_11ac ||
                     sap_phymode == eCSR_DOT11_MODE_11ac_ONLY) {
                /*11AC only 80/40/20 Mhz supported in Rome */
                if (pMac->roam.configParam.nVhtChannelWidth ==
                                        (WNI_CFG_VHT_CHANNEL_WIDTH_80MHZ + 1)) {
                    sap_hbw = HALF_BW_OF(eCSR_BW_80MHz_VAL);
                    if (chb == (PHY_QUADRUPLE_CHANNEL_20MHZ_LOW_40MHZ_LOW-1))
                        sap_cch = CSR_GET_HT80_PLUS_LL_CCH(sap_ch);
                    else if (chb ==
                                 (PHY_QUADRUPLE_CHANNEL_20MHZ_HIGH_40MHZ_LOW-1))
                        sap_cch = CSR_GET_HT80_PLUS_HL_CCH(sap_ch);
                    else if (chb ==
                                 (PHY_QUADRUPLE_CHANNEL_20MHZ_LOW_40MHZ_HIGH-1))
                        sap_cch = CSR_GET_HT80_MINUS_LH_CCH(sap_ch);
                    else if (chb ==
                                (PHY_QUADRUPLE_CHANNEL_20MHZ_HIGH_40MHZ_HIGH-1))
                        sap_cch = CSR_GET_HT80_MINUS_HH_CCH(sap_ch);
                } else {
                    sap_hbw = HALF_BW_OF(eCSR_BW_40MHz_VAL);
                    if (chb == (PHY_QUADRUPLE_CHANNEL_20MHZ_LOW_40MHZ_LOW-1))
                        sap_cch = CSR_GET_HT40_PLUS_CCH(sap_ch);
                    else if (chb ==
                                 (PHY_QUADRUPLE_CHANNEL_20MHZ_HIGH_40MHZ_LOW-1))
                        sap_cch = CSR_GET_HT40_MINUS_CCH(sap_ch);
                    else if (chb ==
                                 (PHY_QUADRUPLE_CHANNEL_20MHZ_LOW_40MHZ_HIGH-1))
                        sap_cch = CSR_GET_HT40_PLUS_CCH(sap_ch);
                    else if (chb ==
                                (PHY_QUADRUPLE_CHANNEL_20MHZ_HIGH_40MHZ_HIGH-1))
                        sap_cch = CSR_GET_HT40_MINUS_CCH(sap_ch);
                }
            }
#endif
        }
        sap_cfreq = vos_chan_to_freq(sap_cch);
    }

    for( i = 0; i < CSR_ROAM_SESSION_MAX; i++ ) {
        if( !CSR_IS_SESSION_VALID( pMac, i ) )
            continue;

        pSession = CSR_GET_SESSION( pMac, i );

        if (NULL != pSession->pCurRoamProfile) {
            if (((pSession->pCurRoamProfile->csrPersona == VOS_STA_MODE) ||
                       (pSession->pCurRoamProfile->csrPersona ==
                            VOS_P2P_CLIENT_MODE)) &&
                       (pSession->connectState ==
                            eCSR_ASSOC_STATE_TYPE_INFRA_ASSOCIATED)) {
                intf_ch = pSession->connectedProfile.operationChannel;
                csrGetChFromHTProfile(pMac,
                        &pSession->connectedProfile.HTProfile, intf_ch,
                        &intf_cfreq, &intf_hbw);
            } else if(((pSession->pCurRoamProfile->csrPersona ==
                            VOS_P2P_GO_MODE) ||
                       (pSession->pCurRoamProfile->csrPersona ==
                            VOS_STA_SAP_MODE)) &&
                       (pSession->connectState !=
                             eCSR_ASSOC_STATE_TYPE_NOT_CONNECTED)) {
                      if (pSession->ch_switch_in_progress)
                          continue;
                /*
                 * if conc_custom_rule1 is defined then we don't want p2pgo to
                 * follow SAP's channel or SAP to follow P2PGO's channel.
                 */
                if (0 == pMac->roam.configParam.conc_custom_rule1 &&
                    0 == pMac->roam.configParam.conc_custom_rule2) {
                    if (sap_ch == 0) {
                        sap_ch = pSession->connectedProfile.operationChannel;
                        csrGetChFromHTProfile(pMac,
                             &pSession->connectedProfile.HTProfile, sap_ch,
                             &sap_cfreq, &sap_hbw);
                    } else if (sap_ch !=
                               pSession->connectedProfile.operationChannel) {
                        intf_ch = pSession->connectedProfile.operationChannel;
                        csrGetChFromHTProfile(pMac,
                             &pSession->connectedProfile.HTProfile, intf_ch,
                             &intf_cfreq, &intf_hbw);
                    }
                } else {
                    if (sap_ch == 0 &&
                        (pSession->pCurRoamProfile->csrPersona ==
                             VOS_STA_SAP_MODE)) {
                        sap_ch = pSession->connectedProfile.operationChannel;
                        csrGetChFromHTProfile(pMac,
                             &pSession->connectedProfile.HTProfile, sap_ch,
                             &sap_cfreq, &sap_hbw);
                    }
                }
            }
        }
    }


    if (intf_ch && sap_ch != intf_ch &&
       cc_switch_mode != VOS_MCC_TO_SCC_SWITCH_FORCE) {
         sap_lfreq = sap_cfreq - sap_hbw;
         sap_hfreq = sap_cfreq + sap_hbw;
         intf_lfreq = intf_cfreq -intf_hbw;
         intf_hfreq = intf_cfreq +intf_hbw;

         smsLog(pMac, LOGE,
         FL("\nSAP:  OCH: %03d OCF: %d CCH: %03d CF: %d BW: %d LF: %d HF: %d\n"
            "INTF: OCH: %03d OCF: %d CCH: %03d CF: %d BW: %d LF: %d HF: %d"),
                 sap_ch, vos_chan_to_freq(sap_ch), vos_freq_to_chan(sap_cfreq),
                 sap_cfreq, sap_hbw*2, sap_lfreq, sap_hfreq,
                 intf_ch, vos_chan_to_freq(intf_ch),
                 vos_freq_to_chan(intf_cfreq),
                 intf_cfreq, intf_hbw*2, intf_lfreq, intf_hfreq);

         if (!(
                ((sap_lfreq > intf_lfreq && sap_lfreq < intf_hfreq) ||
                   (sap_hfreq > intf_lfreq && sap_hfreq < intf_hfreq))
             || ((intf_lfreq > sap_lfreq && intf_lfreq < sap_hfreq) ||
                   (intf_hfreq > sap_lfreq && intf_hfreq < sap_hfreq))
            )) {
             intf_ch = 0;
         }
    }
    else if (!pMac->roam.configParam.band_switch_enable &&
             intf_ch && sap_ch!= intf_ch &&
             cc_switch_mode == VOS_MCC_TO_SCC_SWITCH_FORCE) {
             if (!((intf_ch < 14 && sap_ch < 14) ||
                 (intf_ch > 14 && sap_ch > 14))) {
                 intf_ch = 0;
             }
    }else if (intf_ch == sap_ch)
         intf_ch = 0;

   smsLog(pMac, LOGE, FL("##Concurrent Channels %s Interfering"), intf_ch == 0 ?
                         "Not" : "Are" );
  return intf_ch;
}

/**
 * csr_create_sap_session_info() - create session info based on
 *    the input chan and  phymode
 * @pMac: tpAniSirGlobal ptr
 * @sap_phymode: requesting phymode.
 * @sap_ch: requesting channel number
 * @session_info: information returned.
 *
 * Return: TRUE if any session info returned
 */
tANI_BOOLEAN csr_create_sap_session_info(
	tHalHandle hHal,
	eCsrPhyMode sap_phymode,
	v_U16_t sap_ch,
	session_info_t *session_info)
{
	tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
	v_U8_t chb = PHY_SINGLE_CHANNEL_CENTERED;
	v_U16_t sap_hbw = 0, sap_cfreq = 0;
	v_U16_t sap_lfreq, sap_hfreq, sap_cch;

	sap_cch = sap_ch;
	sap_hbw = HALF_BW_OF(eCSR_BW_20MHz_VAL);

	if (sap_ch > MAX_2_4GHZ_CHANNEL)
		chb = pMac->roam.configParam.channelBondingMode5GHz;
	else
		chb = pMac->roam.configParam.channelBondingMode24GHz;
	if (!chb)
		goto RET;

	if (sap_phymode == eCSR_DOT11_MODE_11n ||
		sap_phymode == eCSR_DOT11_MODE_11n_ONLY) {

		sap_hbw = HALF_BW_OF(eCSR_BW_40MHz_VAL);
		if (chb == PHY_DOUBLE_CHANNEL_LOW_PRIMARY)
			sap_cch = CSR_GET_HT40_PLUS_CCH(sap_ch);
		else if (chb == PHY_DOUBLE_CHANNEL_HIGH_PRIMARY)
			sap_cch = CSR_GET_HT40_MINUS_CCH(sap_ch);
	}
#ifdef WLAN_FEATURE_11AC
	else if (sap_phymode == eCSR_DOT11_MODE_11ac ||
		sap_phymode == eCSR_DOT11_MODE_11ac_ONLY) {
		/*
		 * 11AC only 80/40/20 Mhz supported in Rome
		 */
		if (pMac->roam.configParam.nVhtChannelWidth ==
			(WNI_CFG_VHT_CHANNEL_WIDTH_80MHZ + 1)) {
			sap_hbw = HALF_BW_OF(eCSR_BW_80MHz_VAL);
			if (chb ==
				(PHY_QUADRUPLE_CHANNEL_20MHZ_LOW_40MHZ_LOW
				- 1))
			    sap_cch = CSR_GET_HT80_PLUS_LL_CCH(sap_ch);
			else if (chb ==
				(PHY_QUADRUPLE_CHANNEL_20MHZ_HIGH_40MHZ_LOW
				- 1))
				sap_cch = CSR_GET_HT80_PLUS_HL_CCH(sap_ch);
			else if (chb ==
				(PHY_QUADRUPLE_CHANNEL_20MHZ_LOW_40MHZ_HIGH
				- 1))
				sap_cch = CSR_GET_HT80_MINUS_LH_CCH(sap_ch);
			else if (chb ==
				(PHY_QUADRUPLE_CHANNEL_20MHZ_HIGH_40MHZ_HIGH
				- 1))
				sap_cch = CSR_GET_HT80_MINUS_HH_CCH(sap_ch);
		} else {
			sap_hbw = HALF_BW_OF(eCSR_BW_40MHz_VAL);
			if (chb ==
				(PHY_QUADRUPLE_CHANNEL_20MHZ_LOW_40MHZ_LOW
				- 1))
				sap_cch = CSR_GET_HT40_PLUS_CCH(sap_ch);
			else if (chb ==
				(PHY_QUADRUPLE_CHANNEL_20MHZ_HIGH_40MHZ_LOW
				- 1))
				sap_cch = CSR_GET_HT40_MINUS_CCH(sap_ch);
			else if (chb ==
				(PHY_QUADRUPLE_CHANNEL_20MHZ_LOW_40MHZ_HIGH
				- 1))
				sap_cch = CSR_GET_HT40_PLUS_CCH(sap_ch);
			else if (chb ==
				(PHY_QUADRUPLE_CHANNEL_20MHZ_HIGH_40MHZ_HIGH
				- 1))
				sap_cch = CSR_GET_HT40_MINUS_CCH(sap_ch);
		}
	}
#endif
RET:
	sap_cfreq = vos_chan_to_freq(sap_cch);
	sap_lfreq = sap_cfreq - sap_hbw;
	sap_hfreq = sap_cfreq + sap_hbw;
	if (sap_ch > MAX_2_4GHZ_CHANNEL)
		session_info->band = eCSR_BAND_5G;
	else
		session_info->band = eCSR_BAND_24;
	session_info->och = sap_ch;
	session_info->lfreq = sap_lfreq;
	session_info->hfreq = sap_hfreq;
	session_info->cfreq = sap_cfreq;
	session_info->hbw = sap_hbw;
	session_info->con_mode = VOS_STA_SAP_MODE;
	return TRUE;
}
/**
 * csr_find_sta_session_info() - get sta active session info
 * @pMac: tpAniSirGlobal ptr
 * @session_info: information returned.
 *
 * Return: TRUE if sta session info returned
 */
tANI_BOOLEAN csr_find_sta_session_info(
	tHalHandle hHal,
	session_info_t *info)
{
	tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
	tCsrRoamSession *pSession = NULL;
	v_U8_t i = 0;
	tpPESession psessionEntry;

	for( i = 0; i < CSR_ROAM_SESSION_MAX; i++ ) {
		if( !CSR_IS_SESSION_VALID( pMac, i ) )
			continue;
		pSession = CSR_GET_SESSION( pMac, i );
		if (NULL == pSession->pCurRoamProfile)
			continue;
		if (((pSession->pCurRoamProfile->csrPersona ==
				VOS_STA_MODE) ||
			 (pSession->pCurRoamProfile->csrPersona ==
				VOS_P2P_CLIENT_MODE)) &&
			(pSession->connectState ==
				eCSR_ASSOC_STATE_TYPE_INFRA_ASSOCIATED)) {
			if(vos_is_ch_switch_with_csa_enabled()){
				psessionEntry = peFindSessionBySessionId(pMac,
						pMac->lim.limTimers.gLimChannelSwitchTimer.sessionId);
				if (psessionEntry && LIM_IS_STA_ROLE(psessionEntry)) {
					info->och = psessionEntry->gLimChannelSwitch.primaryChannel;
					smsLog(pMac, LOGP,
						FL("SAP channel switch with CSA enabled (SAP new ch: %d)"), info->och);
				}else{
					info->och =
						pSession->connectedProfile.operationChannel;
				}
			}else{
				info->och =
					pSession->connectedProfile.operationChannel;
			}
			csrGetChFromHTProfile(pMac,
				&pSession->connectedProfile.HTProfile,
				info->och, &info->cfreq, &info->hbw);
			info->lfreq = info->cfreq - info->hbw;
			info->hfreq = info->cfreq + info->hbw;
			if (info->och > MAX_2_4GHZ_CHANNEL)
				info->band = eCSR_BAND_5G;
			else
				info->band = eCSR_BAND_24;
			info->con_mode = VOS_STA_MODE;
			return eANI_BOOLEAN_TRUE;
		}
	}
	return eANI_BOOLEAN_FALSE;
}
/**
 * csr_find_all_session_info() - get all active session info
 * @pMac: tpAniSirGlobal ptr
 * @session_info: information returned.
 * @session_count: number of session
 *
 * Return: TRUE if any session info returned
 */
tANI_BOOLEAN csr_find_all_session_info(
	tHalHandle hHal,
	session_info_t *session_info,
	v_U8_t *session_count)
{
	tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
	tCsrRoamSession *pSession = NULL;
	v_U8_t i = 0;
	v_U8_t count = 0;

	for( i = 0; i < CSR_ROAM_SESSION_MAX; i++ ) {
		if( !CSR_IS_SESSION_VALID( pMac, i ) )
			continue;
		pSession = CSR_GET_SESSION( pMac, i );
		if (NULL == pSession->pCurRoamProfile)
			continue;
		if ((((pSession->pCurRoamProfile->csrPersona ==
				VOS_STA_MODE) ||
			(pSession->pCurRoamProfile->csrPersona ==
				VOS_P2P_CLIENT_MODE)) &&
			(pSession->connectState ==
				eCSR_ASSOC_STATE_TYPE_INFRA_ASSOCIATED)) ||
			(((pSession->pCurRoamProfile->csrPersona ==
				VOS_P2P_GO_MODE) ||
			(pSession->pCurRoamProfile->csrPersona ==
				VOS_STA_SAP_MODE)||
			(pSession->pCurRoamProfile->csrPersona ==
				VOS_IBSS_MODE)) &&
			(pSession->connectState !=
				eCSR_ASSOC_STATE_TYPE_NOT_CONNECTED))) {
			session_info_t *info = &session_info[count++];
			info->och =
				pSession->connectedProfile.operationChannel;
			csrGetChFromHTProfile(pMac,
				&pSession->connectedProfile.HTProfile,
				info->och, &info->cfreq, &info->hbw);
			info->lfreq = info->cfreq - info->hbw;
			info->hfreq = info->cfreq + info->hbw;
			if ((pSession->pCurRoamProfile->csrPersona ==
					VOS_STA_MODE) ||
				(pSession->pCurRoamProfile->csrPersona ==
					VOS_P2P_CLIENT_MODE))
				info->con_mode = VOS_STA_MODE;
			else
				info->con_mode = VOS_STA_SAP_MODE;
			if (info->och > MAX_2_4GHZ_CHANNEL)
				info->band = eCSR_BAND_5G;
			else
				info->band = eCSR_BAND_24;
		}
	}
	*session_count = count;
	return count != 0;
}
#endif

tANI_BOOLEAN csrIsAllSessionDisconnected( tpAniSirGlobal pMac )
{
    tANI_U32 i;
    tANI_BOOLEAN fRc = eANI_BOOLEAN_TRUE;

    for( i = 0; i < CSR_ROAM_SESSION_MAX; i++ )
    {
        if( CSR_IS_SESSION_VALID( pMac, i ) && !csrIsConnStateDisconnected( pMac, i ) )
        {
            fRc = eANI_BOOLEAN_FALSE;
            break;
        }
    }

    return ( fRc );
}

tANI_BOOLEAN csrIsStaSessionConnected( tpAniSirGlobal pMac )
{
    tANI_U32 i;
    tANI_BOOLEAN fRc = eANI_BOOLEAN_FALSE;
    tCsrRoamSession *pSession = NULL;
    tANI_U32 countSta = 0;

    for( i = 0; i < CSR_ROAM_SESSION_MAX; i++ )
    {
        if( CSR_IS_SESSION_VALID( pMac, i ) && !csrIsConnStateDisconnected( pMac, i ) )
        {
            pSession = CSR_GET_SESSION( pMac, i );

            if (NULL != pSession->pCurRoamProfile)
            {
                if (pSession->pCurRoamProfile->csrPersona == VOS_STA_MODE) {
                    countSta++;
                }
            }
        }
    }

    /* return TRUE if one of the following conditions is TRUE:
     * - more than one STA session connected
     */
    if ( countSta > 0) {
        fRc = eANI_BOOLEAN_TRUE;
    }

    return( fRc );
}

tANI_BOOLEAN csrIsP2pOrSapSessionConnected(tpAniSirGlobal pMac)
{
    tANI_U32 i;
    tANI_BOOLEAN fRc = eANI_BOOLEAN_FALSE;
    tCsrRoamSession *pSession = NULL;
    tANI_U32 countP2pCli = 0;
    tANI_U32 countP2pGo = 0;
    tANI_U32 countSAP = 0;

    for( i = 0; i < CSR_ROAM_SESSION_MAX; i++ )
    {
        if( CSR_IS_SESSION_VALID( pMac, i ) && !csrIsConnStateDisconnected( pMac, i ) )
        {
            pSession = CSR_GET_SESSION( pMac, i );

            if (NULL != pSession->pCurRoamProfile)
            {
                if (pSession->pCurRoamProfile->csrPersona == VOS_P2P_CLIENT_MODE) {
                    countP2pCli++;
                }

                if (pSession->pCurRoamProfile->csrPersona == VOS_P2P_GO_MODE) {
                    countP2pGo++;
                }
                if (pSession->pCurRoamProfile->csrPersona ==
                                                   VOS_STA_SAP_MODE) {
                    countSAP++;
                }

            }
        }
    }

    /* return TRUE if one of the following conditions is TRUE:
     * - at least one P2P CLI session is connected
     * - at least one P2P GO session is connected
     */
    if ((countP2pCli > 0) || (countP2pGo > 0 ) || (countSAP > 0)) {
        fRc = eANI_BOOLEAN_TRUE;
    }

    return( fRc );
}

tANI_BOOLEAN csrIsAnySessionConnected( tpAniSirGlobal pMac )
{
    tANI_U32 i, count;
    tANI_BOOLEAN fRc = eANI_BOOLEAN_FALSE;

    count = 0;
    for( i = 0; i < CSR_ROAM_SESSION_MAX; i++ )
    {
        if( CSR_IS_SESSION_VALID( pMac, i ) && !csrIsConnStateDisconnected( pMac, i ) )
        {
            count++;
        }
    }

    if (count > 0)
    {
        fRc = eANI_BOOLEAN_TRUE;
    }
    return( fRc );
}

tANI_BOOLEAN csrIsInfraConnected( tpAniSirGlobal pMac )
{
    tANI_U32 i;
    tANI_BOOLEAN fRc = eANI_BOOLEAN_FALSE;

    for( i = 0; i < CSR_ROAM_SESSION_MAX; i++ )
    {
        if( CSR_IS_SESSION_VALID( pMac, i ) && csrIsConnStateConnectedInfra( pMac, i ) )
        {
            fRc = eANI_BOOLEAN_TRUE;
            break;
        }
    }

    return ( fRc );
}

tANI_BOOLEAN csrIsConcurrentInfraConnected( tpAniSirGlobal pMac )
{
    tANI_U32 i, noOfConnectedInfra = 0;

    tANI_BOOLEAN fRc = eANI_BOOLEAN_FALSE;

    for( i = 0; i < CSR_ROAM_SESSION_MAX; i++ )
    {
        if( CSR_IS_SESSION_VALID( pMac, i ) && csrIsConnStateConnectedInfra( pMac, i ) )
        {
            ++noOfConnectedInfra;
        }
    }

    // More than one Infra Sta Connected
    if(noOfConnectedInfra > 1)
    {
        fRc = eANI_BOOLEAN_TRUE;
    }

    return ( fRc );
}

tANI_BOOLEAN csrIsIBSSStarted( tpAniSirGlobal pMac )
{
    tANI_U32 i;
    tANI_BOOLEAN fRc = eANI_BOOLEAN_FALSE;

    for( i = 0; i < CSR_ROAM_SESSION_MAX; i++ )
    {
        if( CSR_IS_SESSION_VALID( pMac, i ) && csrIsConnStateIbss( pMac, i ) )
        {
            fRc = eANI_BOOLEAN_TRUE;
            break;
        }
    }

    return ( fRc );
}


tANI_BOOLEAN csrIsBTAMPStarted( tpAniSirGlobal pMac )
{
    tANI_U32 i;
    tANI_BOOLEAN fRc = eANI_BOOLEAN_FALSE;

    for( i = 0; i < CSR_ROAM_SESSION_MAX; i++ )
    {
        if( CSR_IS_SESSION_VALID( pMac, i ) && csrIsConnStateConnectedWds( pMac, i ) )
        {
            fRc = eANI_BOOLEAN_TRUE;
            break;
        }
    }

    return ( fRc );
}

tANI_BOOLEAN csrIsConcurrentSessionRunning( tpAniSirGlobal pMac )
{
    tANI_U32 sessionId, noOfCocurrentSession = 0;
    eCsrConnectState connectState;

    tANI_BOOLEAN fRc = eANI_BOOLEAN_FALSE;

    for( sessionId = 0; sessionId < CSR_ROAM_SESSION_MAX; sessionId++ )
    {
        if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
        {
           connectState =  pMac->roam.roamSession[sessionId].connectState;
           if( (eCSR_ASSOC_STATE_TYPE_INFRA_ASSOCIATED == connectState) ||
               (eCSR_ASSOC_STATE_TYPE_INFRA_CONNECTED == connectState) ||
               (eCSR_ASSOC_STATE_TYPE_INFRA_DISCONNECTED == connectState) )
           {
              ++noOfCocurrentSession;
           }
        }
    }

    // More than one session is Up and Running
    if(noOfCocurrentSession > 1)
    {
        fRc = eANI_BOOLEAN_TRUE;
    }

    return ( fRc );
}

tANI_BOOLEAN csrIsInfraApStarted( tpAniSirGlobal pMac )
{
    tANI_U32 sessionId;
    tANI_BOOLEAN fRc = eANI_BOOLEAN_FALSE;

    for( sessionId = 0; sessionId < CSR_ROAM_SESSION_MAX; sessionId++ )
    {
        if( CSR_IS_SESSION_VALID( pMac, sessionId ) && (csrIsConnStateConnectedInfraAp(pMac, sessionId)) )
        {
            fRc = eANI_BOOLEAN_TRUE;
            break;
        }
    }

    return ( fRc );

}

tANI_BOOLEAN csrIsBTAMP( tpAniSirGlobal pMac, tANI_U32 sessionId )
{
    return ( csrIsConnStateConnectedWds( pMac, sessionId ) );
}


tANI_BOOLEAN csrIsConnStateDisconnected(tpAniSirGlobal pMac, tANI_U32 sessionId)
{
    return (eCSR_ASSOC_STATE_TYPE_NOT_CONNECTED == pMac->roam.roamSession[sessionId].connectState);
}

tANI_BOOLEAN csrIsValidMcConcurrentSession(tpAniSirGlobal pMac, tANI_U32 sessionId,
                                                  tSirBssDescription *pBssDesc)
{
    tCsrRoamSession *pSession = NULL;
    eAniBoolean status = eANI_BOOLEAN_FALSE;

    //Check for MCC support
    if (!pMac->roam.configParam.fenableMCCMode)
    {
        return status;
    }

    //Validate BeaconInterval
    if( CSR_IS_SESSION_VALID( pMac, sessionId ) )
    {
        pSession = CSR_GET_SESSION( pMac, sessionId );
        if (NULL != pSession->pCurRoamProfile)
        {
            if (csrIsconcurrentsessionValid (pMac, sessionId,
                                       pSession->pCurRoamProfile->csrPersona)
                                       == eHAL_STATUS_SUCCESS )
            {
                if (csrValidateMCCBeaconInterval( pMac, pBssDesc->channelId,
                               &pBssDesc->beaconInterval, sessionId,
                               pSession->pCurRoamProfile->csrPersona)
                               != eHAL_STATUS_SUCCESS)
                {
                    status = eANI_BOOLEAN_FALSE;
                }
                else
                {
                    status = eANI_BOOLEAN_TRUE;
                }
            }
            else
            {
                status = eANI_BOOLEAN_FALSE;
            }
         }
     }
    return status;
}

static tSirMacCapabilityInfo csrGetBssCapabilities( tSirBssDescription *pSirBssDesc )
{
    tSirMacCapabilityInfo dot11Caps;

    //tSirMacCapabilityInfo is 16-bit
    pal_get_U16( (tANI_U8 *)&pSirBssDesc->capabilityInfo, (tANI_U16 *)&dot11Caps );

    return( dot11Caps );
}

tANI_BOOLEAN csrIsInfraBssDesc( tSirBssDescription *pSirBssDesc )
{
    tSirMacCapabilityInfo dot11Caps = csrGetBssCapabilities( pSirBssDesc );

    return( (tANI_BOOLEAN)dot11Caps.ess );
}


tANI_BOOLEAN csrIsIbssBssDesc( tSirBssDescription *pSirBssDesc )
{
    tSirMacCapabilityInfo dot11Caps = csrGetBssCapabilities( pSirBssDesc );

    return( (tANI_BOOLEAN)dot11Caps.ibss );
}

tANI_BOOLEAN csrIsQoSBssDesc( tSirBssDescription *pSirBssDesc )
{
    tSirMacCapabilityInfo dot11Caps = csrGetBssCapabilities( pSirBssDesc );

    return( (tANI_BOOLEAN)dot11Caps.qos );
}

tANI_BOOLEAN csrIsPrivacy( tSirBssDescription *pSirBssDesc )
{
    tSirMacCapabilityInfo dot11Caps = csrGetBssCapabilities( pSirBssDesc );

    return( (tANI_BOOLEAN)dot11Caps.privacy );
}


tANI_BOOLEAN csrIs11dSupported(tpAniSirGlobal pMac)
{
    return(pMac->roam.configParam.Is11dSupportEnabled);
}


tANI_BOOLEAN csrIs11hSupported(tpAniSirGlobal pMac)
{
    return(pMac->roam.configParam.Is11hSupportEnabled);
}


tANI_BOOLEAN csrIs11eSupported(tpAniSirGlobal pMac)
{
    return(pMac->roam.configParam.Is11eSupportEnabled);
}

tANI_BOOLEAN csrIsMCCSupported ( tpAniSirGlobal pMac )
{
   return(pMac->roam.configParam.fenableMCCMode);

}

tANI_BOOLEAN csrIsWmmSupported(tpAniSirGlobal pMac)
{
    if(eCsrRoamWmmNoQos == pMac->roam.configParam.WMMSupportMode)
    {
       return eANI_BOOLEAN_FALSE;
    }
    else
    {
       return eANI_BOOLEAN_TRUE;
    }
}




//pIes is the IEs for pSirBssDesc2
tANI_BOOLEAN csrIsSsidEqual( tHalHandle hHal, tSirBssDescription *pSirBssDesc1,
                             tSirBssDescription *pSirBssDesc2, tDot11fBeaconIEs *pIes2 )
{
    tANI_BOOLEAN fEqual = FALSE;
    tSirMacSSid Ssid1, Ssid2;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tDot11fBeaconIEs *pIes1 = NULL;
    tDot11fBeaconIEs *pIesLocal = pIes2;

    do {
        if( ( NULL == pSirBssDesc1 ) || ( NULL == pSirBssDesc2 ) ) break;
        if( !pIesLocal && !HAL_STATUS_SUCCESS(csrGetParsedBssDescriptionIEs(pMac, pSirBssDesc2, &pIesLocal)) )
        {
            smsLog(pMac, LOGE, FL("  fail to parse IEs"));
            break;
        }
        if(!HAL_STATUS_SUCCESS(csrGetParsedBssDescriptionIEs(pMac, pSirBssDesc1, &pIes1)))
        {
            break;
        }
        if( ( !pIes1->SSID.present ) || ( !pIesLocal->SSID.present ) ) break;
        if ( pIes1->SSID.num_ssid != pIesLocal->SSID.num_ssid ) break;
        vos_mem_copy(Ssid1.ssId, pIes1->SSID.ssid, pIes1->SSID.num_ssid);
        vos_mem_copy(Ssid2.ssId, pIesLocal->SSID.ssid, pIesLocal->SSID.num_ssid);

        fEqual = vos_mem_compare(Ssid1.ssId, Ssid2.ssId, pIesLocal->SSID.num_ssid);

    } while( 0 );
    if(pIes1)
    {
        vos_mem_free(pIes1);
    }
    if( pIesLocal && !pIes2 )
    {
        vos_mem_free(pIesLocal);
    }

    return( fEqual );
}


//pIes can be passed in as NULL if the caller doesn't have one prepared
tANI_BOOLEAN csrIsBssDescriptionWme( tHalHandle hHal, tSirBssDescription *pSirBssDesc, tDot11fBeaconIEs *pIes )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    // Assume that WME is found...
    tANI_BOOLEAN fWme = TRUE;
    tDot11fBeaconIEs *pIesTemp = pIes;

    do
    {
        if(pIesTemp == NULL)
        {
            if( !HAL_STATUS_SUCCESS(csrGetParsedBssDescriptionIEs(pMac, pSirBssDesc, &pIesTemp)) )
            {
                fWme = FALSE;
                break;
            }
        }
        // if the Wme Info IE is found, then WME is supported...
        if ( CSR_IS_QOS_BSS(pIesTemp) ) break;
        // if none of these are found, then WME is NOT supported...
        fWme = FALSE;
    } while( 0 );
    if( !csrIsWmmSupported( pMac ) && fWme)
    {
        if( !pIesTemp->HTCaps.present )
        {
            fWme = FALSE;
        }
    }
    if( ( pIes == NULL ) && ( NULL != pIesTemp ) )
    {
        //we allocate memory here so free it before returning
        vos_mem_free(pIesTemp);
    }

    return( fWme );
}

eCsrMediaAccessType csrGetQoSFromBssDesc( tHalHandle hHal, tSirBssDescription *pSirBssDesc,
                                          tDot11fBeaconIEs *pIes )
{
    eCsrMediaAccessType qosType = eCSR_MEDIUM_ACCESS_DCF;

    if (NULL == pIes)
    {
       VOS_ASSERT( pIes != NULL );
       return( qosType );
    }

    do
   {
        // if we find WMM in the Bss Description, then we let this
        // override and use WMM.
        if ( csrIsBssDescriptionWme( hHal, pSirBssDesc, pIes ) )
        {
            qosType = eCSR_MEDIUM_ACCESS_WMM_eDCF_DSCP;
        }
        else
        {
            // if the QoS bit is on, then the AP is advertising 11E QoS...
            if (csrIsQoSBssDesc(pSirBssDesc)) {
                qosType = eCSR_MEDIUM_ACCESS_11e_eDCF;
            } else {
                qosType = eCSR_MEDIUM_ACCESS_DCF;
            }
            // scale back based on the types turned on for the adapter...
            if ( eCSR_MEDIUM_ACCESS_11e_eDCF == qosType && !csrIs11eSupported( hHal ) )
            {
                qosType = eCSR_MEDIUM_ACCESS_DCF;
            }
        }

    } while(0);

    return( qosType );
}




//Caller allocates memory for pIEStruct
eHalStatus csrParseBssDescriptionIEs(tHalHandle hHal, tSirBssDescription *pBssDesc, tDot11fBeaconIEs *pIEStruct)
{
    eHalStatus status = eHAL_STATUS_FAILURE;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    int ieLen = (int)GET_IE_LEN_IN_BSS(pBssDesc->length);

    if(ieLen > 0 && pIEStruct)
    {
        if(!DOT11F_FAILED(dot11fUnpackBeaconIEs( pMac, (tANI_U8 *)pBssDesc->ieFields, ieLen, pIEStruct )))
        {
            status = eHAL_STATUS_SUCCESS;
        }
    }

    return (status);
}


//This function will allocate memory for the parsed IEs to the caller. Caller must free the memory
//after it is done with the data only if this function succeeds
eHalStatus csrGetParsedBssDescriptionIEs(tHalHandle hHal, tSirBssDescription *pBssDesc, tDot11fBeaconIEs **ppIEStruct)
{
    eHalStatus status = eHAL_STATUS_INVALID_PARAMETER;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    if(pBssDesc && ppIEStruct)
    {
        *ppIEStruct = vos_mem_malloc(sizeof(tDot11fBeaconIEs));
        if ( (*ppIEStruct) != NULL)
        {
            vos_mem_set((void *)*ppIEStruct, sizeof(tDot11fBeaconIEs), 0);
            status = csrParseBssDescriptionIEs(hHal, pBssDesc, *ppIEStruct);
            if(!HAL_STATUS_SUCCESS(status))
            {
                vos_mem_free(*ppIEStruct);
                *ppIEStruct = NULL;
            }
        }
        else
        {
            smsLog( pMac, LOGE, FL(" failed to allocate memory") );
            VOS_ASSERT( 0 );
            return eHAL_STATUS_FAILURE;
        }
    }

    return (status);
}




tANI_BOOLEAN csrIsNULLSSID( tANI_U8 *pBssSsid, tANI_U8 len )
{
    tANI_BOOLEAN fNullSsid = FALSE;

    tANI_U32 SsidLength;
    tANI_U8 *pSsidStr;

    do
    {
        if ( 0 == len )
        {
            fNullSsid = TRUE;
            break;
        }

        //Consider 0 or space for hidden SSID
        if ( 0 == pBssSsid[0] )
        {
             fNullSsid = TRUE;
             break;
        }

        SsidLength = len;
        pSsidStr = pBssSsid;

        while ( SsidLength )
        {
            if( *pSsidStr )
                break;

            pSsidStr++;
            SsidLength--;
        }

        if( 0 == SsidLength )
        {
            fNullSsid = TRUE;
            break;
        }
    }
    while( 0 );

    return fNullSsid;
}


tANI_U32 csrGetFragThresh( tHalHandle hHal )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    return pMac->roam.configParam.FragmentationThreshold;
}

tANI_U32 csrGetRTSThresh( tHalHandle hHal )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    return pMac->roam.configParam.RTSThreshold;
}

eCsrPhyMode csrTranslateToPhyModeFromBssDesc( tSirBssDescription *pSirBssDesc )
{
    eCsrPhyMode phyMode;

    switch ( pSirBssDesc->nwType )
    {
        case eSIR_11A_NW_TYPE:
            phyMode = eCSR_DOT11_MODE_11a;
            break;

        case eSIR_11B_NW_TYPE:
            phyMode = eCSR_DOT11_MODE_11b;
            break;

        case eSIR_11G_NW_TYPE:
            phyMode = eCSR_DOT11_MODE_11g;
            break;

        case eSIR_11N_NW_TYPE:
            phyMode = eCSR_DOT11_MODE_11n;
            break;
#ifdef WLAN_FEATURE_11AC
        case eSIR_11AC_NW_TYPE:
        default:
            phyMode = eCSR_DOT11_MODE_11ac;
#else
        default:
            phyMode = eCSR_DOT11_MODE_11n;
#endif
            break;
    }
    return( phyMode );
}


tANI_U32 csrTranslateToWNICfgDot11Mode(tpAniSirGlobal pMac, eCsrCfgDot11Mode csrDot11Mode)
{
    tANI_U32 ret;

    switch(csrDot11Mode)
    {
    case eCSR_CFG_DOT11_MODE_AUTO:
        smsLog(pMac, LOGW, FL("  Warning: sees eCSR_CFG_DOT11_MODE_AUTO "));
#ifdef WLAN_FEATURE_11AC
        if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
             ret = WNI_CFG_DOT11_MODE_11AC;
        else
             ret = WNI_CFG_DOT11_MODE_11N;
#else
        //We cannot decide until now.
        ret = WNI_CFG_DOT11_MODE_11N;
#endif
        break;
    case eCSR_CFG_DOT11_MODE_11A:
        ret = WNI_CFG_DOT11_MODE_11A;
        break;
    case eCSR_CFG_DOT11_MODE_11B:
        ret = WNI_CFG_DOT11_MODE_11B;
        break;
    case eCSR_CFG_DOT11_MODE_11G:
        ret = WNI_CFG_DOT11_MODE_11G;
        break;
    case eCSR_CFG_DOT11_MODE_11N:
        ret = WNI_CFG_DOT11_MODE_11N;
        break;
    case eCSR_CFG_DOT11_MODE_11G_ONLY:
       ret = WNI_CFG_DOT11_MODE_11G_ONLY;
       break;
    case eCSR_CFG_DOT11_MODE_11N_ONLY:
       ret = WNI_CFG_DOT11_MODE_11N_ONLY;
       break;

#ifdef WLAN_FEATURE_11AC
     case eCSR_CFG_DOT11_MODE_11AC_ONLY:
        if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
             ret = WNI_CFG_DOT11_MODE_11AC_ONLY;
        else
             ret = WNI_CFG_DOT11_MODE_11N;
        break;
     case eCSR_CFG_DOT11_MODE_11AC:
        if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
             ret = WNI_CFG_DOT11_MODE_11AC;
        else
             ret = WNI_CFG_DOT11_MODE_11N;
       break;
#endif
    default:
        smsLog(pMac, LOGW, FL("doesn't expect %d as csrDo11Mode"), csrDot11Mode);
        if(eCSR_BAND_24 == pMac->roam.configParam.eBand)
        {
            ret = WNI_CFG_DOT11_MODE_11G;
        }
        else
        {
            ret = WNI_CFG_DOT11_MODE_11A;
        }
        break;
    }

    return (ret);
}


//This function should only return the super set of supported modes. 11n implies 11b/g/a/n.
eHalStatus csrGetPhyModeFromBss(tpAniSirGlobal pMac, tSirBssDescription *pBSSDescription,
                                eCsrPhyMode *pPhyMode, tDot11fBeaconIEs *pIes)
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    eCsrPhyMode phyMode = csrTranslateToPhyModeFromBssDesc(pBSSDescription);

    if (pIes) {
        if (pIes->HTCaps.present) {
            phyMode = eCSR_DOT11_MODE_11n;
#ifdef WLAN_FEATURE_11AC
        if (IS_BSS_VHT_CAPABLE(pIes->VHTCaps) ||
                        IS_BSS_VHT_CAPABLE(pIes->vendor2_ie.VHTCaps))
                 phyMode = eCSR_DOT11_MODE_11ac;
#endif
        }

        *pPhyMode = phyMode;
    }

    return (status);

}


//This function returns the correct eCSR_CFG_DOT11_MODE is the two phyModes matches
//bssPhyMode is the mode derived from the BSS description
//f5GhzBand is derived from the channel id of BSS description
tANI_BOOLEAN csrGetPhyModeInUse( eCsrPhyMode phyModeIn, eCsrPhyMode bssPhyMode, tANI_BOOLEAN f5GhzBand,
                                 eCsrCfgDot11Mode *pCfgDot11ModeToUse )
{
    tANI_BOOLEAN fMatch = FALSE;
    eCsrCfgDot11Mode cfgDot11Mode;

    cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N; // to suppress compiler warning

    switch( phyModeIn )
    {
        case eCSR_DOT11_MODE_abg:   //11a or 11b or 11g
            if( f5GhzBand )
            {
                fMatch = TRUE;
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11A;
            }
            else if( eCSR_DOT11_MODE_11b == bssPhyMode )
            {
                fMatch = TRUE;
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
            }
            else
            {
                fMatch = TRUE;
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
            }
            break;

        case eCSR_DOT11_MODE_11a:   //11a
            if( f5GhzBand )
            {
                fMatch = TRUE;
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11A;
            }
            break;

        case eCSR_DOT11_MODE_11g:
            if(!f5GhzBand)
            {
                if( eCSR_DOT11_MODE_11b == bssPhyMode )
                {
                    fMatch = TRUE;
                    cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
                }
                else
                {
                    fMatch = TRUE;
                    cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
                }
            }
            break;

        case eCSR_DOT11_MODE_11g_ONLY:
            if( eCSR_DOT11_MODE_11g == bssPhyMode )
            {
                fMatch = TRUE;
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
            }
            break;

        case eCSR_DOT11_MODE_11b:
            if( !f5GhzBand )
            {
                fMatch = TRUE;
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
            }
            break;

        case eCSR_DOT11_MODE_11b_ONLY:
            if( eCSR_DOT11_MODE_11b == bssPhyMode )
            {
                fMatch = TRUE;
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
            }
            break;

        case eCSR_DOT11_MODE_11n:
            fMatch = TRUE;
            switch(bssPhyMode)
            {
            case eCSR_DOT11_MODE_11g:
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
                break;
            case eCSR_DOT11_MODE_11b:
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
                break;
            case eCSR_DOT11_MODE_11a:
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11A;
                break;
            case eCSR_DOT11_MODE_11n:
#ifdef WLAN_FEATURE_11AC
            case eCSR_DOT11_MODE_11ac:
#endif
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
                break;

            default:
#ifdef WLAN_FEATURE_11AC
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
#else
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
#endif
                break;
            }
            break;

        case eCSR_DOT11_MODE_11n_ONLY:
            if ((eCSR_DOT11_MODE_11n == bssPhyMode)) {
                fMatch = TRUE;
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
            }

            break;
#ifdef WLAN_FEATURE_11AC
        case eCSR_DOT11_MODE_11ac:
            fMatch = TRUE;
            switch(bssPhyMode)
            {
            case eCSR_DOT11_MODE_11g:
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
                break;
            case eCSR_DOT11_MODE_11b:
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
                break;
            case eCSR_DOT11_MODE_11a:
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11A;
                break;
            case eCSR_DOT11_MODE_11n:
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
                break;
            case eCSR_DOT11_MODE_11ac:
            default:
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
                break;
            }
            break;

        case eCSR_DOT11_MODE_11ac_ONLY:
            if ((eCSR_DOT11_MODE_11ac == bssPhyMode)) {
                fMatch = TRUE;
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
            }
            break;
#endif

        default:
            fMatch = TRUE;
            switch(bssPhyMode)
            {
            case eCSR_DOT11_MODE_11g:
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
                break;
            case eCSR_DOT11_MODE_11b:
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
                break;
            case eCSR_DOT11_MODE_11a:
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11A;
                break;
            case eCSR_DOT11_MODE_11n:
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
                break;
#ifdef WLAN_FEATURE_11AC
            case eCSR_DOT11_MODE_11ac:
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
                break;
#endif
            default:
                cfgDot11Mode = eCSR_CFG_DOT11_MODE_AUTO;
                break;
            }
            break;
    }

    if ( fMatch && pCfgDot11ModeToUse )
    {
#ifdef WLAN_FEATURE_11AC
        if(cfgDot11Mode == eCSR_CFG_DOT11_MODE_11AC && (!IS_FEATURE_SUPPORTED_BY_FW(DOT11AC)))
        {
            *pCfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11N;
        }
        else
#endif
        {
            *pCfgDot11ModeToUse = cfgDot11Mode;
        }
    }
    return( fMatch );
}


//This function decides whether the one of the bit of phyMode is matching the mode in the BSS and allowed by the user
//setting, pMac->roam.configParam.uCfgDot11Mode. It returns the mode that fits the criteria.
tANI_BOOLEAN csrIsPhyModeMatch( tpAniSirGlobal pMac, tANI_U32 phyMode,
                                tSirBssDescription *pSirBssDesc, tCsrRoamProfile *pProfile,
                                eCsrCfgDot11Mode *pReturnCfgDot11Mode,
                                tDot11fBeaconIEs *pIes)
{
    tANI_BOOLEAN fMatch = FALSE;
    eCsrPhyMode phyModeInBssDesc = eCSR_DOT11_MODE_AUTO, phyMode2;
    eCsrCfgDot11Mode cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_AUTO;
    tANI_U32 bitMask, loopCount;

    if (HAL_STATUS_SUCCESS(csrGetPhyModeFromBss(pMac, pSirBssDesc,
                                                &phyModeInBssDesc, pIes))) {
        if ((0 == phyMode) || (eCSR_DOT11_MODE_AUTO & phyMode)) {
            if (eCSR_CFG_DOT11_MODE_ABG == pMac->roam.configParam.uCfgDot11Mode) {
                phyMode = eCSR_DOT11_MODE_abg;
            } else if (eCSR_CFG_DOT11_MODE_AUTO ==
                                       pMac->roam.configParam.uCfgDot11Mode) {
#ifdef WLAN_FEATURE_11AC
                    phyMode = eCSR_DOT11_MODE_11ac;
#else
                    phyMode = eCSR_DOT11_MODE_11n;
#endif
            } else {
                //user's pick
                phyMode = pMac->roam.configParam.phyMode;
            }
        }

        if ((0 == phyMode) || (eCSR_DOT11_MODE_AUTO & phyMode)) {
            if (0 != phyMode) {
                if (eCSR_DOT11_MODE_AUTO & phyMode) {
                    phyMode2 = eCSR_DOT11_MODE_AUTO & phyMode;
                }
            } else {
                phyMode2 = phyMode;
            }
            fMatch = csrGetPhyModeInUse( phyMode2, phyModeInBssDesc, CSR_IS_CHANNEL_5GHZ(pSirBssDesc->channelId),
                                                &cfgDot11ModeToUse );
        }
        else
        {
            bitMask = 1;
            loopCount = 0;
            while(loopCount < eCSR_NUM_PHY_MODE)
            {
                if(0 != ( phyMode2 = (phyMode & (bitMask << loopCount++)) ))
                {
                    fMatch = csrGetPhyModeInUse( phyMode2, phyModeInBssDesc, CSR_IS_CHANNEL_5GHZ(pSirBssDesc->channelId),
                                        &cfgDot11ModeToUse );
                    if(fMatch) break;
                }
            }
        }
        if ( fMatch && pReturnCfgDot11Mode )
        {
            if( pProfile )
            {
                /* IEEE 11n spec (8.4.3): HT STA shall eliminate TKIP as a
                 * choice for the pairwise cipher suite if CCMP is advertised
                 * by the AP or if the AP included an HT capabilities element
                 * in its Beacons and Probe Response.
                 */
                if ((!CSR_IS_11n_ALLOWED(pProfile->negotiatedUCEncryptionType)) &&
                    ((eCSR_CFG_DOT11_MODE_11N == cfgDot11ModeToUse) ||
#ifdef WLAN_FEATURE_11AC
                     (eCSR_CFG_DOT11_MODE_11AC == cfgDot11ModeToUse)
#endif
                     )) {
                    /* We cannot do 11n here */
                    if (!CSR_IS_CHANNEL_5GHZ(pSirBssDesc->channelId)) {
                        cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11G;
                    } else {
                        cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11A;
                    }
                }
            }
            *pReturnCfgDot11Mode = cfgDot11ModeToUse;
        }
    }

    return( fMatch );
}


eCsrCfgDot11Mode csrFindBestPhyMode( tpAniSirGlobal pMac, tANI_U32 phyMode )
{
    eCsrCfgDot11Mode cfgDot11ModeToUse;
    eCsrBand eBand = pMac->roam.configParam.eBand;


    if ((0 == phyMode) ||
#ifdef WLAN_FEATURE_11AC
        (eCSR_DOT11_MODE_11ac & phyMode) ||
#endif
        (eCSR_DOT11_MODE_AUTO & phyMode))
    {
#ifdef WLAN_FEATURE_11AC
        if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
        {
           cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11AC;
        }
        else
#endif
        {
           /* Default to 11N mode if user has configured 11ac mode
            * and FW doesn't supports 11ac mode .
            */
           cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11N;
        }
    }
    else
    {
        if( ( eCSR_DOT11_MODE_11n | eCSR_DOT11_MODE_11n_ONLY ) & phyMode )
        {
            cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11N;
        }
        else if ( eCSR_DOT11_MODE_abg & phyMode )
        {
            if( eCSR_BAND_24 != eBand )
            {
                cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11A;
            }
            else
            {
                cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11G;
            }
        }
        else if(eCSR_DOT11_MODE_11a & phyMode)
        {
            cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11A;
        }
        else if( ( eCSR_DOT11_MODE_11g | eCSR_DOT11_MODE_11g_ONLY ) & phyMode )
        {
            cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11G;
        }
        else
        {
            cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11B;
        }
    }

    return ( cfgDot11ModeToUse );
}




tANI_U32 csrGet11hPowerConstraint( tHalHandle hHal, tDot11fIEPowerConstraints *pPowerConstraint )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tANI_U32 localPowerConstraint = 0;

    // check if .11h support is enabled, if not, the power constraint is 0.
    if(pMac->roam.configParam.Is11hSupportEnabled && pPowerConstraint->present)
    {
        localPowerConstraint = pPowerConstraint->localPowerConstraints;
    }

    return( localPowerConstraint );
}


tANI_BOOLEAN csrIsProfileWpa( tCsrRoamProfile *pProfile )
{
    tANI_BOOLEAN fWpaProfile = FALSE;

    switch ( pProfile->negotiatedAuthType )
    {
        case eCSR_AUTH_TYPE_WPA:
        case eCSR_AUTH_TYPE_WPA_PSK:
        case eCSR_AUTH_TYPE_WPA_NONE:
#ifdef FEATURE_WLAN_ESE
        case eCSR_AUTH_TYPE_CCKM_WPA:
#endif
            fWpaProfile = TRUE;
            break;

        default:
            fWpaProfile = FALSE;
            break;
    }

    if ( fWpaProfile )
    {
        switch ( pProfile->negotiatedUCEncryptionType )
        {
            case eCSR_ENCRYPT_TYPE_WEP40:
            case eCSR_ENCRYPT_TYPE_WEP104:
            case eCSR_ENCRYPT_TYPE_TKIP:
            case eCSR_ENCRYPT_TYPE_AES:
                fWpaProfile = TRUE;
                break;

            default:
                fWpaProfile = FALSE;
                break;
        }
    }
    return( fWpaProfile );
}

tANI_BOOLEAN csrIsProfileRSN( tCsrRoamProfile *pProfile )
{
    tANI_BOOLEAN fRSNProfile = FALSE;

    switch ( pProfile->negotiatedAuthType )
    {
        case eCSR_AUTH_TYPE_RSN:
        case eCSR_AUTH_TYPE_RSN_PSK:
#ifdef WLAN_FEATURE_VOWIFI_11R
        case eCSR_AUTH_TYPE_FT_RSN:
        case eCSR_AUTH_TYPE_FT_RSN_PSK:
#endif
#ifdef FEATURE_WLAN_ESE
        case eCSR_AUTH_TYPE_CCKM_RSN:
#endif
#ifdef WLAN_FEATURE_11W
        case eCSR_AUTH_TYPE_RSN_PSK_SHA256:
        case eCSR_AUTH_TYPE_RSN_8021X_SHA256:
#endif
            fRSNProfile = TRUE;
            break;
#ifdef WLAN_FEATURE_FILS_SK
        /* fallthrough */
        case eCSR_AUTH_TYPE_FILS_SHA256:
        case eCSR_AUTH_TYPE_FILS_SHA384:
        case eCSR_AUTH_TYPE_FT_FILS_SHA256:
        case eCSR_AUTH_TYPE_FT_FILS_SHA384:
            fRSNProfile = true;
            break;
#endif
        case eCSR_AUTH_TYPE_SAE:
            fRSNProfile = true;
            break;
        case eCSR_AUTH_TYPE_OWE:
            fRSNProfile = true;
            break;

        default:
            fRSNProfile = FALSE;
            break;
    }

    if ( fRSNProfile )
    {
        switch ( pProfile->negotiatedUCEncryptionType )
        {
            // !!REVIEW - For WPA2, use of RSN IE mandates
            // use of AES as encryption. Here, we qualify
            // even if encryption type is WEP or TKIP
            case eCSR_ENCRYPT_TYPE_WEP40:
            case eCSR_ENCRYPT_TYPE_WEP104:
            case eCSR_ENCRYPT_TYPE_TKIP:
            case eCSR_ENCRYPT_TYPE_AES:
                fRSNProfile = TRUE;
                break;

            default:
                fRSNProfile = FALSE;
                break;
        }
    }
    return( fRSNProfile );
}

eHalStatus
csrIsconcurrentsessionValid(tpAniSirGlobal pMac,tANI_U32 cursessionId,
                                 tVOS_CON_MODE currBssPersona)
{
    tANI_U32 sessionId = 0;
    tANI_U8 automotive_support_enable =
        (pMac->roam.configParam.conc_custom_rule1 |
         pMac->roam.configParam.conc_custom_rule2);
#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH
    bool ap_p2pgo_concurrency_enable =
                 pMac->roam.configParam.ap_p2pgo_concurrency_enable;
#endif
    tVOS_CON_MODE bss_persona;
    eCsrConnectState connect_state;

    for (sessionId = 0; sessionId < CSR_ROAM_SESSION_MAX; sessionId++) {
         if (cursessionId != sessionId ) {
             if (!CSR_IS_SESSION_VALID( pMac, sessionId )) {
                 continue;
             }
             bss_persona =
                 pMac->roam.roamSession[sessionId].bssParams.bssPersona;
             connect_state =
                 pMac->roam.roamSession[sessionId].connectState;

             switch (currBssPersona) {

             case VOS_STA_MODE:
                     VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_INFO,
                               FL("** STA session **"));
                     return eHAL_STATUS_SUCCESS;

             case VOS_STA_SAP_MODE:
#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH
                     if ((VOS_MCC_TO_SCC_SWITCH_FORCE ==
                             pMac->roam.configParam.cc_switch_mode) &&
                         (ap_p2pgo_concurrency_enable) &&
                         (bss_persona == VOS_P2P_GO_MODE) &&
                         (connect_state !=
                                eCSR_ASSOC_STATE_TYPE_NOT_CONNECTED)) {
                         VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_INFO,
                             FL("Start AP session concurrency with P2P-GO"));
                         return eHAL_STATUS_SUCCESS;
                     } else
#endif
                     if (((bss_persona == VOS_P2P_GO_MODE) &&
                             (0 == automotive_support_enable) &&
                             (connect_state !=
                                    eCSR_ASSOC_STATE_TYPE_NOT_CONNECTED)) ||
                             ((bss_persona == VOS_IBSS_MODE) &&
                             (connect_state !=
                                eCSR_ASSOC_STATE_TYPE_IBSS_DISCONNECTED))) {
                             VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_ERROR,
                                 FL("Can't start multiple beaconing role"));
                         return eHAL_STATUS_FAILURE;
                     }
                     break;

             case VOS_P2P_GO_MODE:
                     if ((bss_persona == VOS_P2P_GO_MODE) && (connect_state !=
                                eCSR_ASSOC_STATE_TYPE_NOT_CONNECTED)) {
                         VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_ERROR,
                                FL(" ****P2P GO mode already exists ****"));
                         return eHAL_STATUS_FAILURE;
                     }
#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH
                     else if ((VOS_MCC_TO_SCC_SWITCH_FORCE ==
                                   pMac->roam.configParam.cc_switch_mode) &&
                               (ap_p2pgo_concurrency_enable) &&
                               (bss_persona == VOS_STA_SAP_MODE) &&
                               (connect_state !=
                                       eCSR_ASSOC_STATE_TYPE_NOT_CONNECTED)) {
                         VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_INFO,
                                FL("Start P2P-GO session concurrency with AP"));
                         return eHAL_STATUS_SUCCESS;
                     }
#endif
                     else if (((bss_persona == VOS_STA_SAP_MODE) &&
                                 (connect_state !=
                                  eCSR_ASSOC_STATE_TYPE_NOT_CONNECTED) &&
                                 (0 == automotive_support_enable)) ||
                                ((bss_persona == VOS_IBSS_MODE) &&
                                 (connect_state !=
                                  eCSR_ASSOC_STATE_TYPE_IBSS_DISCONNECTED))) {
                         VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_ERROR,
                                   FL("Can't start multiple beaconing role"));
                         return eHAL_STATUS_FAILURE;
                     }
                     break;
             case VOS_IBSS_MODE:
                     if ((bss_persona == VOS_IBSS_MODE) &&
                         (connect_state !=
                                eCSR_ASSOC_STATE_TYPE_IBSS_CONNECTED)) {
                         VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_ERROR,
                                FL(" ****IBSS mode already exists ****"));
                         return eHAL_STATUS_FAILURE;
                     } else if (((bss_persona == VOS_P2P_GO_MODE) ||
                                 (bss_persona == VOS_STA_SAP_MODE)) &&
                                (connect_state
                                   != eCSR_ASSOC_STATE_TYPE_NOT_CONNECTED)) {
                         VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_ERROR,
                                FL("**Cannot start Multiple Beaconing Role**"));
                         return eHAL_STATUS_FAILURE;
                     }
                     break;
             case VOS_P2P_CLIENT_MODE:
                     VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_INFO,
                               FL("**P2P-Client session**"));
                     return eHAL_STATUS_SUCCESS;
             case VOS_NDI_MODE:
                     if (bss_persona != VOS_STA_MODE) {
                         VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_ERROR,
                             FL("***NDI mode can co-exist only with STA ***"));
                         return eHAL_STATUS_FAILURE;
                     }
                     break;
             default :
                     VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_ERROR,
                               FL("**Persona not handled = %d**"),
                               currBssPersona);
                     break;
             }
        }
    }
    return eHAL_STATUS_SUCCESS;

}

eHalStatus csrUpdateMCCp2pBeaconInterval(tpAniSirGlobal pMac)
{
    tANI_U32 sessionId = 0;

    //If MCC is not supported just break and return SUCCESS
    if ( !pMac->roam.configParam.fenableMCCMode){
        return eHAL_STATUS_FAILURE;
    }

    for (sessionId = 0; sessionId < CSR_ROAM_SESSION_MAX; sessionId++ )
    {
        /* If GO in MCC support different beacon interval,
         * change the BI of the P2P-GO */
        if (pMac->roam.roamSession[sessionId].bssParams.bssPersona
                              == VOS_P2P_GO_MODE)
        {
           /* Handle different BI scenario based on the configuration set.
            * If Config is set to 0x02 then Disconnect all the P2P clients
            * associated. If config is set to 0x04 then update the BI
            * without disconnecting all the clients
            */
           if ((pMac->roam.configParam.fAllowMCCGODiffBI == 0x04) &&
               (pMac->roam.roamSession[sessionId].bssParams.updatebeaconInterval))
           {
               return csrSendChngMCCBeaconInterval( pMac, sessionId);
           }
           //If the configuration of fAllowMCCGODiffBI is set to other than 0x04
           else if ( pMac->roam.roamSession[sessionId].bssParams.updatebeaconInterval)
           {
               return csrRoamCallCallback(pMac, sessionId, NULL, 0, eCSR_ROAM_DISCONNECT_ALL_P2P_CLIENTS, eCSR_ROAM_RESULT_NONE);
           }
        }
    }
    return eHAL_STATUS_FAILURE;
}

tANI_U16 csrCalculateMCCBeaconInterval(tpAniSirGlobal pMac, tANI_U16 sta_bi, tANI_U16 go_gbi)
{
    tANI_U8 num_beacons = 0;
    tANI_U8 is_multiple = 0;
    tANI_U16 go_cbi = 0;
    tANI_U16 go_fbi = 0;
    tANI_U16 sta_cbi = 0;

    //If GO's given beacon Interval is less than 100
    if(go_gbi < 100)
       go_cbi = 100;
    //if GO's given beacon Interval is greater than or equal to 100
    else
       go_cbi = 100 + (go_gbi % 100);

    if ( sta_bi == 0 )
    {
        /* There is possibility to receive zero as value.
           Which will cause divide by zero. Hence initialize with 100
        */
        sta_bi =  100;
        smsLog(pMac, LOGW,
            FL("sta_bi 2nd parameter is zero, initialize to %d"), sta_bi);
    }

    // check, if either one is multiple of another
    if (sta_bi > go_cbi)
    {
        is_multiple = !(sta_bi % go_cbi);
    }
    else
    {
        is_multiple = !(go_cbi % sta_bi);
    }
    // if it is multiple, then accept GO's beacon interval range [100,199] as it  is
    if (is_multiple)
    {
        return go_cbi;
    }
    //else , if it is not multiple, then then check for number of beacons to be
    //inserted based on sta BI
    num_beacons = sta_bi / 100;
    if (num_beacons)
    {
        // GO's final beacon interval will be aligned to sta beacon interval, but
        //in the range of [100, 199].
        sta_cbi = sta_bi / num_beacons;
        go_fbi = sta_cbi;
    }
    else
    {
        // if STA beacon interval is less than 100, use GO's change bacon interval
        //instead of updating to STA's beacon interval.
        go_fbi = go_cbi;
    }
    return go_fbi;
}

eHalStatus csrValidateMCCBeaconInterval(tpAniSirGlobal pMac, tANI_U8 channelId,
                                     tANI_U16 *beaconInterval, tANI_U32 cursessionId,
                                     tVOS_CON_MODE currBssPersona)
{
    tANI_U32 sessionId = 0;
    tANI_U16 new_beaconInterval = 0;

    //If MCC is not supported just break
    if (!pMac->roam.configParam.fenableMCCMode){
        return eHAL_STATUS_FAILURE;
    }

    for (sessionId = 0; sessionId < CSR_ROAM_SESSION_MAX; sessionId++ )
    {
        if (cursessionId != sessionId )
        {
            if (!CSR_IS_SESSION_VALID( pMac, sessionId ))
            {
                continue;
            }

            switch (currBssPersona)
            {
                case VOS_STA_MODE:
                    if (pMac->roam.roamSession[sessionId].pCurRoamProfile &&
                       (pMac->roam.roamSession[sessionId].pCurRoamProfile->csrPersona
                                      == VOS_P2P_CLIENT_MODE)) //check for P2P client mode
                    {
                        smsLog(pMac, LOG1, FL(" Beacon Interval Validation not required for STA/CLIENT"));
                    }
                    //IF SAP has started and STA wants to connect on different channel MCC should
                    //MCC should not be enabled so making it false to enforce on same channel
                    else if (pMac->roam.roamSession[sessionId].bssParams.bssPersona
                                      == VOS_STA_SAP_MODE)
                    {
                        if (pMac->roam.roamSession[sessionId].bssParams.operationChn
                                                        != channelId )
                        {
                            smsLog(pMac, LOGE, FL("*** MCC with SAP+STA sessions ****"));
                            return eHAL_STATUS_SUCCESS;
                        }
                    }
                    else if (pMac->roam.roamSession[sessionId].bssParams.bssPersona
                                      == VOS_P2P_GO_MODE) //Check for P2P go scenario
                    {
                        /* if GO in MCC support different beacon interval,
                         * change the BI of the P2P-GO */
                       if ((pMac->roam.roamSession[sessionId].bssParams.operationChn
                                != channelId ) &&
                           (pMac->roam.roamSession[sessionId].bssParams.beaconInterval
                                != *beaconInterval))
                       {
                           /* if GO in MCC support different beacon interval, return success */
                           if ( pMac->roam.configParam.fAllowMCCGODiffBI == 0x01)
                           {
                               return eHAL_STATUS_SUCCESS;
                           }
                           // Send only Broadcast disassoc and update beaconInterval
                           //If configuration is set to 0x04 then dont
                           // disconnect all the station
                           else if ((pMac->roam.configParam.fAllowMCCGODiffBI == 0x02) ||
                                   (pMac->roam.configParam.fAllowMCCGODiffBI == 0x04))
                           {
                               //Check to pass the right beacon Interval
                               if (pMac->roam.configParam.conc_custom_rule1 ||
                                     pMac->roam.configParam.conc_custom_rule2) {
                                     new_beaconInterval = CSR_CUSTOM_CONC_GO_BI;
                               } else {
                                     new_beaconInterval =
                                        csrCalculateMCCBeaconInterval(pMac,
                                                                      *beaconInterval,
                                                                      pMac->roam.roamSession[sessionId].bssParams.beaconInterval);
                               }
                               smsLog(pMac, LOG1, FL(" Peer AP BI : %d, new Beacon Interval: %d"),*beaconInterval,new_beaconInterval );
                               //Update the becon Interval
                               if (new_beaconInterval != pMac->roam.roamSession[sessionId].bssParams.beaconInterval)
                               {
                                   //Update the beaconInterval now
                                   smsLog(pMac, LOGE, FL(" Beacon Interval got changed config used: %d\n"),
                                                 pMac->roam.configParam.fAllowMCCGODiffBI);

                                   pMac->roam.roamSession[sessionId].bssParams.beaconInterval = new_beaconInterval;
                                   pMac->roam.roamSession[sessionId].bssParams.updatebeaconInterval = eANI_BOOLEAN_TRUE;
                                    return csrUpdateMCCp2pBeaconInterval(pMac);
                               }
                               return eHAL_STATUS_SUCCESS;
                           }
                           //Disconnect the P2P session
                           else if (pMac->roam.configParam.fAllowMCCGODiffBI == 0x03)
                           {
                               pMac->roam.roamSession[sessionId].bssParams.updatebeaconInterval =  eANI_BOOLEAN_FALSE;
                               return csrRoamCallCallback(pMac, sessionId, NULL, 0, eCSR_ROAM_SEND_P2P_STOP_BSS, eCSR_ROAM_RESULT_NONE);
                           }
                           else
                           {
                               smsLog(pMac, LOGE, FL("BeaconInterval is different cannot connect to preferred AP..."));
                               return eHAL_STATUS_FAILURE;
                           }
                        }
                    }
                    break;

                case VOS_P2P_CLIENT_MODE:
                    if (pMac->roam.roamSession[sessionId].pCurRoamProfile &&
                      (pMac->roam.roamSession[sessionId].pCurRoamProfile->csrPersona
                                                                == VOS_STA_MODE)) //check for P2P client mode
                    {
                        smsLog(pMac, LOG1, FL(" Ignore Beacon Interval Validation..."));
                    }
                    //IF SAP has started and STA wants to connect on different channel MCC should
                    //MCC should not be enabled so making it false to enforce on same channel
                    else if (pMac->roam.roamSession[sessionId].bssParams.bssPersona
                                      == VOS_STA_SAP_MODE)
                    {
                        if (pMac->roam.roamSession[sessionId].bssParams.operationChn
                                                        != channelId )
                        {
#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH
                            if (VOS_MCC_TO_SCC_SWITCH_FORCE ==
                                            pMac->roam.configParam.cc_switch_mode &&
                                pMac->roam.configParam.ap_p2pclient_concur_enable)
                            {
                                smsLog(pMac, LOG1, FL("SAP + CLIENT for MCC to SCC"));
                                return eHAL_STATUS_SUCCESS;
                            } else
#endif
                            {
                                smsLog(pMac, LOGE,
                                        FL("***MCC is not enabled for SAP + CLIENT****"));
                                return eHAL_STATUS_FAILURE;
                            }
                        }
                    }
                    else if (pMac->roam.roamSession[sessionId].bssParams.bssPersona
                                    == VOS_P2P_GO_MODE) //Check for P2P go scenario
                    {
                        if ((pMac->roam.roamSession[sessionId].bssParams.operationChn
                                != channelId ) &&
                            (pMac->roam.roamSession[sessionId].bssParams.beaconInterval
                                != *beaconInterval))
                        {
                            smsLog(pMac, LOGE, FL("BeaconInterval is different cannot connect to P2P_GO network ..."));
                            return eHAL_STATUS_FAILURE;
                        }
                    }
                    break;

                case VOS_STA_SAP_MODE :
                    break;

                case VOS_P2P_GO_MODE :
                {
                    if (pMac->roam.roamSession[sessionId].pCurRoamProfile  &&
                      ((pMac->roam.roamSession[sessionId].pCurRoamProfile->csrPersona
                            == VOS_P2P_CLIENT_MODE) ||
                      (pMac->roam.roamSession[sessionId].pCurRoamProfile->csrPersona
                            == VOS_STA_MODE))) //check for P2P_client scenario
                    {
                        if ((pMac->roam.roamSession[sessionId].connectedProfile.operationChannel
                               == 0 )&&
                           (pMac->roam.roamSession[sessionId].connectedProfile.beaconInterval
                               == 0))
                        {
                            continue;
                        }


                        if (csrIsConnStateConnectedInfra(pMac, sessionId) &&
                           (pMac->roam.roamSession[sessionId].connectedProfile.operationChannel
                                != channelId ) &&
                           (pMac->roam.roamSession[sessionId].connectedProfile.beaconInterval
                                != *beaconInterval))
                        {
                            /*
                             * Updated beaconInterval should be used only when
                             * we are starting a new BSS not in-case of client
                             * or STA case
                             */
                            /* Calculate beacon Interval for P2P-GO
                               in-case of MCC */
                           if (pMac->roam.configParam.conc_custom_rule1 ||
                                  pMac->roam.configParam.conc_custom_rule2) {
                                  new_beaconInterval = CSR_CUSTOM_CONC_GO_BI;
                           } else {
                                  new_beaconInterval =
                                      csrCalculateMCCBeaconInterval(pMac,
                                                                    pMac->roam.roamSession[sessionId].connectedProfile.beaconInterval,
                                                                    *beaconInterval);
                           }
                            if(*beaconInterval != new_beaconInterval)
                                *beaconInterval = new_beaconInterval;
                            return eHAL_STATUS_SUCCESS;
                         }
                    }
                }
                break;

                default :
                    smsLog(pMac, LOGE, FL(" Persona not supported : %d"),currBssPersona);
                    return eHAL_STATUS_FAILURE;
            }
        }
    }

    return eHAL_STATUS_SUCCESS;
}

#ifdef WLAN_FEATURE_VOWIFI_11R
/* Function to return TRUE if the auth type is 11r */
tANI_BOOLEAN csrIsAuthType11r( eCsrAuthType AuthType, tANI_U8 mdiePresent)
{
    switch ( AuthType )
    {
        case eCSR_AUTH_TYPE_OPEN_SYSTEM:
            if(mdiePresent)
                return TRUE;
            break;
        case eCSR_AUTH_TYPE_FT_RSN_PSK:
        case eCSR_AUTH_TYPE_FT_RSN:
            return TRUE;
            break;
        default:
            break;
    }
    return FALSE;
}

/* Function to return TRUE if the profile is 11r */
tANI_BOOLEAN csrIsProfile11r( tCsrRoamProfile *pProfile )
{
    return csrIsAuthType11r( pProfile->negotiatedAuthType, pProfile->MDID.mdiePresent );
}

#endif

#ifdef FEATURE_WLAN_ESE

/* Function to return TRUE if the auth type is ESE */
tANI_BOOLEAN csrIsAuthTypeESE( eCsrAuthType AuthType )
{
    switch ( AuthType )
    {
        case eCSR_AUTH_TYPE_CCKM_WPA:
        case eCSR_AUTH_TYPE_CCKM_RSN:
            return TRUE;
            break;
        default:
            break;
    }
    return FALSE;
}

/* Function to return TRUE if the profile is ESE */
tANI_BOOLEAN csrIsProfileESE( tCsrRoamProfile *pProfile )
{
    return (csrIsAuthTypeESE( pProfile->negotiatedAuthType ));
}

#endif

#ifdef FEATURE_WLAN_WAPI
tANI_BOOLEAN csrIsProfileWapi( tCsrRoamProfile *pProfile )
{
    tANI_BOOLEAN fWapiProfile = FALSE;

    switch ( pProfile->negotiatedAuthType )
    {
        case eCSR_AUTH_TYPE_WAPI_WAI_CERTIFICATE:
        case eCSR_AUTH_TYPE_WAPI_WAI_PSK:
            fWapiProfile = TRUE;
            break;

        default:
            fWapiProfile = FALSE;
            break;
    }

    if ( fWapiProfile )
    {
        switch ( pProfile->negotiatedUCEncryptionType )
        {
            case eCSR_ENCRYPT_TYPE_WPI:
                fWapiProfile = TRUE;
                break;

            default:
                fWapiProfile = FALSE;
                break;
        }
    }
    return( fWapiProfile );
}

static tANI_BOOLEAN csrIsWapiOuiEqual( tpAniSirGlobal pMac, tANI_U8 *Oui1, tANI_U8 *Oui2 )
{
    return (vos_mem_compare(Oui1, Oui2, CSR_WAPI_OUI_SIZE));
}

static tANI_BOOLEAN csrIsWapiOuiMatch( tpAniSirGlobal pMac, tANI_U8 AllCyphers[][CSR_WAPI_OUI_SIZE],
                                     tANI_U8 cAllCyphers,
                                     tANI_U8 Cypher[],
                                     tANI_U8 Oui[] )
{
    tANI_BOOLEAN fYes = FALSE;
    tANI_U8 idx;

    for ( idx = 0; idx < cAllCyphers; idx++ )
    {
        if ( csrIsWapiOuiEqual( pMac, AllCyphers[ idx ], Cypher ) )
        {
            fYes = TRUE;
            break;
        }
    }

    if ( fYes && Oui )
    {
        vos_mem_copy(Oui, AllCyphers[ idx ], CSR_WAPI_OUI_SIZE);
    }

    return( fYes );
}
#endif /* FEATURE_WLAN_WAPI */

static tANI_BOOLEAN csrIsWpaOuiEqual( tpAniSirGlobal pMac, tANI_U8 *Oui1, tANI_U8 *Oui2 )
{
    return(vos_mem_compare(Oui1, Oui2, CSR_WPA_OUI_SIZE));
}

static tANI_BOOLEAN csrIsOuiMatch( tpAniSirGlobal pMac, tANI_U8 AllCyphers[][CSR_WPA_OUI_SIZE],
                                     tANI_U8 cAllCyphers,
                                     tANI_U8 Cypher[],
                                     tANI_U8 Oui[] )
{
    tANI_BOOLEAN fYes = FALSE;
    tANI_U8 idx;

    for ( idx = 0; idx < cAllCyphers; idx++ )
    {
        if ( csrIsWpaOuiEqual( pMac, AllCyphers[ idx ], Cypher ) )
        {
            fYes = TRUE;
            break;
        }
    }

    if ( fYes && Oui )
    {
        vos_mem_copy(Oui, AllCyphers[ idx ], CSR_WPA_OUI_SIZE);
    }

    return( fYes );
}

static tANI_BOOLEAN csrMatchRSNOUIIndex( tpAniSirGlobal pMac, tANI_U8 AllCyphers[][CSR_RSN_OUI_SIZE],
                                            tANI_U8 cAllCyphers, tANI_U8 ouiIndex,
                                            tANI_U8 Oui[] )
{
    return( csrIsOuiMatch( pMac, AllCyphers, cAllCyphers, csrRSNOui[ouiIndex], Oui ) );

}

#ifdef FEATURE_WLAN_WAPI
static tANI_BOOLEAN csrMatchWapiOUIIndex( tpAniSirGlobal pMac, tANI_U8 AllCyphers[][CSR_WAPI_OUI_SIZE],
                                            tANI_U8 cAllCyphers, tANI_U8 ouiIndex,
                                            tANI_U8 Oui[] )
{
    return( csrIsWapiOuiMatch( pMac, AllCyphers, cAllCyphers, csrWapiOui[ouiIndex], Oui ) );

}
#endif /* FEATURE_WLAN_WAPI */

static tANI_BOOLEAN csrMatchWPAOUIIndex( tpAniSirGlobal pMac, tANI_U8 AllCyphers[][CSR_RSN_OUI_SIZE],
                                            tANI_U8 cAllCyphers, tANI_U8 ouiIndex,
                                            tANI_U8 Oui[] )
{
    return( csrIsOuiMatch( pMac, AllCyphers, cAllCyphers, csrWpaOui[ouiIndex], Oui ) );

}

#ifdef FEATURE_WLAN_WAPI
static tANI_BOOLEAN csrIsAuthWapiCert( tpAniSirGlobal pMac, tANI_U8 AllSuites[][CSR_WAPI_OUI_SIZE],
                                  tANI_U8 cAllSuites,
                                  tANI_U8 Oui[] )
{
    return( csrIsWapiOuiMatch( pMac, AllSuites, cAllSuites, csrWapiOui[1], Oui ) );
}
static tANI_BOOLEAN csrIsAuthWapiPsk( tpAniSirGlobal pMac, tANI_U8 AllSuites[][CSR_WAPI_OUI_SIZE],
                                      tANI_U8 cAllSuites,
                                      tANI_U8 Oui[] )
{
    return( csrIsWapiOuiMatch( pMac, AllSuites, cAllSuites, csrWapiOui[2], Oui ) );
}
#endif /* FEATURE_WLAN_WAPI */

#ifdef WLAN_FEATURE_VOWIFI_11R

/*
 * Function for 11R FT Authentication. We match the FT Authentication Cipher suite
 * here. This matches for FT Auth with the 802.1X exchange.
 *
 */
static tANI_BOOLEAN csrIsFTAuthRSN( tpAniSirGlobal pMac, tANI_U8 AllSuites[][CSR_RSN_OUI_SIZE],
                                  tANI_U8 cAllSuites,
                                  tANI_U8 Oui[] )
{
    return( csrIsOuiMatch( pMac, AllSuites, cAllSuites, csrRSNOui[03], Oui ) );
}

/*
 * Function for 11R FT Authentication. We match the FT Authentication Cipher suite
 * here. This matches for FT Auth with the PSK.
 *
 */
static tANI_BOOLEAN csrIsFTAuthRSNPsk( tpAniSirGlobal pMac, tANI_U8 AllSuites[][CSR_RSN_OUI_SIZE],
                                      tANI_U8 cAllSuites,
                                      tANI_U8 Oui[] )
{
    return( csrIsOuiMatch( pMac, AllSuites, cAllSuites, csrRSNOui[04], Oui ) );
}

#endif

#ifdef FEATURE_WLAN_ESE

/*
 * Function for ESE CCKM AKM Authentication. We match the CCKM AKM Authentication Key Management suite
 * here. This matches for CCKM AKM Auth with the 802.1X exchange.
 *
 */
static tANI_BOOLEAN csrIsEseCckmAuthRSN( tpAniSirGlobal pMac, tANI_U8 AllSuites[][CSR_RSN_OUI_SIZE],
                                  tANI_U8 cAllSuites,
                                  tANI_U8 Oui[] )
{
    return( csrIsOuiMatch( pMac, AllSuites, cAllSuites, csrRSNOui[06], Oui ) );
}

static tANI_BOOLEAN csrIsEseCckmAuthWpa( tpAniSirGlobal pMac, tANI_U8 AllSuites[][CSR_WPA_OUI_SIZE],
                                tANI_U8 cAllSuites,
                                tANI_U8 Oui[] )
{
    return( csrIsOuiMatch( pMac, AllSuites, cAllSuites, csrWpaOui[06], Oui ) );
}

#endif

static tANI_BOOLEAN csrIsAuthRSN( tpAniSirGlobal pMac, tANI_U8 AllSuites[][CSR_RSN_OUI_SIZE],
                                  tANI_U8 cAllSuites,
                                  tANI_U8 Oui[] )
{
    return( csrIsOuiMatch( pMac, AllSuites, cAllSuites, csrRSNOui[01], Oui ) );
}
static tANI_BOOLEAN csrIsAuthRSNPsk( tpAniSirGlobal pMac, tANI_U8 AllSuites[][CSR_RSN_OUI_SIZE],
                                      tANI_U8 cAllSuites,
                                      tANI_U8 Oui[] )
{
    return( csrIsOuiMatch( pMac, AllSuites, cAllSuites, csrRSNOui[02], Oui ) );
}

#ifdef WLAN_FEATURE_11W
static tANI_BOOLEAN csrIsAuthRSNPskSha256( tpAniSirGlobal pMac, tANI_U8 AllSuites[][CSR_RSN_OUI_SIZE],
                                      tANI_U8 cAllSuites,
                                      tANI_U8 Oui[] )
{
    return csrIsOuiMatch( pMac, AllSuites, cAllSuites, csrRSNOui[07], Oui );
}
static tANI_BOOLEAN csrIsAuthRSN8021xSha256(tpAniSirGlobal pMac,
                                            tANI_U8 AllSuites[][CSR_RSN_OUI_SIZE],
                                            tANI_U8 cAllSuites,
                                            tANI_U8 Oui[] )
{
    return csrIsOuiMatch( pMac, AllSuites, cAllSuites, csrRSNOui[8], Oui );
}
#endif

#ifdef WLAN_FEATURE_FILS_SK
/*
 * csr_is_auth_fils_sha256() - check whether oui is fils sha256
 * @mac: Global MAC context
 * @all_suites: pointer to all supported akm suites
 * @suite_count: all supported akm suites count
 * @oui: Oui needs to be matched
 *
 * Return: True if OUI is FILS SHA256, false otherwise
 */
static bool csr_is_auth_fils_sha256(tpAniSirGlobal mac,
                    uint8_t all_suites[][CSR_RSN_OUI_SIZE],
                    uint8_t suite_count, uint8_t oui[])
{
    return csrIsOuiMatch(mac, all_suites, suite_count,
                csrRSNOui[ENUM_FILS_SHA256], oui);
}

/*
 * csr_is_auth_fils_sha384() - check whether oui is fils sha384
 * @mac: Global MAC context
 * @all_suites: pointer to all supported akm suites
 * @suite_count: all supported akm suites count
 * @oui: Oui needs to be matched
 *
 * Return: True if OUI is FILS SHA384, false otherwise
 */
static bool csr_is_auth_fils_sha384(tpAniSirGlobal mac,
                    uint8_t all_suites[][CSR_RSN_OUI_SIZE],
                    uint8_t suite_count, uint8_t oui[])
{
    return csrIsOuiMatch(mac, all_suites, suite_count,
                csrRSNOui[ENUM_FILS_SHA384], oui);
}

/*
 * csr_is_auth_fils_ft_sha256() - check whether oui is fils ft sha256
 * @mac: Global MAC context
 * @all_suites: pointer to all supported akm suites
 * @suite_count: all supported akm suites count
 * @oui: Oui needs to be matched
 *
 * Return: True if OUI is FT FILS SHA256, false otherwise
 */
static bool csr_is_auth_fils_ft_sha256(tpAniSirGlobal mac,
                    uint8_t all_suites[][CSR_RSN_OUI_SIZE],
                    uint8_t suite_count, uint8_t oui[])
{
    return csrIsOuiMatch(mac, all_suites, suite_count,
                csrRSNOui[ENUM_FT_FILS_SHA256], oui);
}

/*
 * csr_is_auth_fils_ft_sha384() - check whether oui is fils ft sha384
 * @mac: Global MAC context
 * @all_suites: pointer to all supported akm suites
 * @suite_count: all supported akm suites count
 * @oui: Oui needs to be matched
 *
 * Return: True if OUI is FT FILS SHA384, false otherwise
 */
static bool csr_is_auth_fils_ft_sha384(tpAniSirGlobal mac,
                    uint8_t all_suites[][CSR_RSN_OUI_SIZE],
                    uint8_t suite_count, uint8_t oui[])
{
    return csrIsOuiMatch(mac, all_suites, suite_count,
                csrRSNOui[ENUM_FT_FILS_SHA384], oui);
}
#endif

#ifdef WLAN_FEATURE_SAE
/**
 * csr_is_auth_wpa_sae() - check whether oui is SAE
 * @mac: Global MAC context
 * @all_suites: pointer to all supported akm suites
 * @suite_count: all supported akm suites count
 * @oui: Oui needs to be matched
 *
 * Return: True if OUI is SAE, false otherwise
 */
static bool csr_is_auth_wpa_sae(tpAniSirGlobal mac,
                                uint8_t all_suites[][CSR_RSN_OUI_SIZE],
                                uint8_t suite_count, uint8_t oui[])
{
	return csrIsOuiMatch(mac, all_suites, suite_count,
			     csrRSNOui[ENUM_SAE], oui);
}
#endif

/*
 * csr_is_auth_wpa_owe() - check whether oui is OWE
 * @mac: Global MAC context
 * @all_suites: pointer to all supported akm suites
 * @suite_count: all supported akm suites count
 * @oui: Oui needs to be matched
 *
 * Return: True if OUI is SAE, false otherwise
 */
static bool csr_is_auth_wpa_owe(tpAniSirGlobal mac,
				uint8_t all_suites[][CSR_RSN_OUI_SIZE],
				uint8_t suite_count, uint8_t oui[])
{
	return csrIsOuiMatch
		(mac, all_suites, suite_count, csrRSNOui[ENUM_OWE], oui);
}

static tANI_BOOLEAN csrIsAuthWpa( tpAniSirGlobal pMac, tANI_U8 AllSuites[][CSR_WPA_OUI_SIZE],
                                tANI_U8 cAllSuites,
                                tANI_U8 Oui[] )
{
    return( csrIsOuiMatch( pMac, AllSuites, cAllSuites, csrWpaOui[01], Oui ) );
}

static tANI_BOOLEAN csrIsAuthWpaPsk( tpAniSirGlobal pMac, tANI_U8 AllSuites[][CSR_WPA_OUI_SIZE],
                                tANI_U8 cAllSuites,
                                tANI_U8 Oui[] )
{
    return( csrIsOuiMatch( pMac, AllSuites, cAllSuites, csrWpaOui[02], Oui ) );
}

tANI_U8 csrGetOUIIndexFromCipher( eCsrEncryptionType enType )
{
    tANI_U8 OUIIndex;

        switch ( enType )
        {
            case eCSR_ENCRYPT_TYPE_WEP40:
            case eCSR_ENCRYPT_TYPE_WEP40_STATICKEY:
                OUIIndex = CSR_OUI_WEP40_OR_1X_INDEX;
                break;
            case eCSR_ENCRYPT_TYPE_WEP104:
            case eCSR_ENCRYPT_TYPE_WEP104_STATICKEY:
                OUIIndex = CSR_OUI_WEP104_INDEX;
                break;
            case eCSR_ENCRYPT_TYPE_TKIP:
                OUIIndex = CSR_OUI_TKIP_OR_PSK_INDEX;
                break;
            case eCSR_ENCRYPT_TYPE_AES:
                OUIIndex = CSR_OUI_AES_INDEX;
                break;
            case eCSR_ENCRYPT_TYPE_NONE:
                OUIIndex = CSR_OUI_USE_GROUP_CIPHER_INDEX;
                break;
#ifdef FEATURE_WLAN_WAPI
           case eCSR_ENCRYPT_TYPE_WPI:
               OUIIndex = CSR_OUI_WAPI_WAI_CERT_OR_SMS4_INDEX;
               break;
#endif /* FEATURE_WLAN_WAPI */
            default: //HOWTO handle this?
                OUIIndex = CSR_OUI_RESERVED_INDEX;
                break;
        }//switch

        return OUIIndex;
}

#ifdef WLAN_FEATURE_FILS_SK
/**
 * csr_is_fils_auth() - update negotiated auth if matches to FILS auth type
 * @mac_ctx: pointer to mac context
 * @authsuites: auth suites
 * @c_auth_suites: auth suites count
 * @authentication: authentication
 * @auth_type: authentication type list
 * @index: current counter
 * @neg_authtype: pointer to negotiated auth
 *
 * Return: None
 */
static void csr_is_fils_auth(tpAniSirGlobal mac_ctx,
    uint8_t authsuites[][CSR_RSN_OUI_SIZE], uint8_t c_auth_suites,
    uint8_t authentication[], tCsrAuthList *auth_type,
    uint8_t index, eCsrAuthType *neg_authtype)
{
    /*
     * TODO Always try with highest security
     * move this down once sha384 is validated
     */
    if (csr_is_auth_fils_sha256(mac_ctx, authsuites,
                c_auth_suites, authentication)) {
        if (eCSR_AUTH_TYPE_FILS_SHA256 ==
                auth_type->authType[index])
            *neg_authtype = eCSR_AUTH_TYPE_FILS_SHA256;
    }
    if ((*neg_authtype == eCSR_AUTH_TYPE_UNKNOWN) &&
            csr_is_auth_fils_sha384(mac_ctx, authsuites,
                c_auth_suites, authentication)) {
        if (eCSR_AUTH_TYPE_FILS_SHA384 ==
                auth_type->authType[index])
            *neg_authtype = eCSR_AUTH_TYPE_FILS_SHA384;
    }
    if ((*neg_authtype == eCSR_AUTH_TYPE_UNKNOWN) &&
            csr_is_auth_fils_ft_sha256(mac_ctx, authsuites,
                c_auth_suites, authentication)) {
        if (eCSR_AUTH_TYPE_FT_FILS_SHA256 ==
                auth_type->authType[index])
            *neg_authtype = eCSR_AUTH_TYPE_FT_FILS_SHA256;
    }
    if ((*neg_authtype == eCSR_AUTH_TYPE_UNKNOWN) &&
            csr_is_auth_fils_ft_sha384(mac_ctx, authsuites,
                c_auth_suites, authentication)) {
        if (eCSR_AUTH_TYPE_FT_FILS_SHA384 ==
                auth_type->authType[index])
            *neg_authtype = eCSR_AUTH_TYPE_FT_FILS_SHA384;
    }
     VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_ERROR,
              FL("negotiated auth type is %d"), *neg_authtype);
}
#else
static void csr_is_fils_auth(tpAniSirGlobal mac_ctx,
    uint8_t authsuites[][CSR_RSN_OUI_SIZE], uint8_t c_auth_suites,
    uint8_t authentication[], tCsrAuthList *auth_type,
    uint8_t index, eCsrAuthType *neg_authtype)
{
}
#endif

#ifdef WLAN_FEATURE_SAE
/**
 * csr_check_sae_auth() - update negotiated auth if matches to SAE auth type
 * @mac_ctx: pointer to mac context
 * @authsuites: auth suites
 * @c_auth_suites: auth suites count
 * @authentication: authentication
 * @auth_type: authentication type list
 * @index: current counter
 * @neg_authtype: pointer to negotiated auth
 *
 * Return: None
 */
static void csr_check_sae_auth(tpAniSirGlobal mac_ctx,
                               uint8_t authsuites[][CSR_RSN_OUI_SIZE],
                               uint8_t c_auth_suites,
                               uint8_t authentication[],
                               tCsrAuthList *auth_type,
                               uint8_t index, eCsrAuthType *neg_authtype)
{
	if ((*neg_authtype == eCSR_AUTH_TYPE_UNKNOWN) &&
		csr_is_auth_wpa_sae(mac_ctx, authsuites,
				    c_auth_suites, authentication)) {
		if (eCSR_AUTH_TYPE_SAE == auth_type->authType[index])
			*neg_authtype = eCSR_AUTH_TYPE_SAE;
	}
	VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_ERROR,
		  FL("negotiated auth type is %d"), *neg_authtype);
}
#else
static void csr_check_sae_auth(tpAniSirGlobal mac_ctx,
                               uint8_t authsuites[][CSR_RSN_OUI_SIZE],
                               uint8_t c_auth_suites,
                               uint8_t authentication[],
                               tCsrAuthList *auth_type,
                               uint8_t index, eCsrAuthType *neg_authtype)
{
}
#endif

bool csr_is_pmkid_found_for_peer(tpAniSirGlobal mac,
				 tCsrRoamSession *session,
				 tSirMacAddr peer_mac_addr,
				 uint8_t *pmkid, uint16_t pmkid_count)
{
	uint32_t i, index;
	uint8_t *session_pmkid;
	tPmkidCacheInfo pmkid_cache;

	vos_mem_zero(&pmkid_cache, sizeof(pmkid_cache));
	vos_mem_copy(pmkid_cache.BSSID, peer_mac_addr, VOS_MAC_ADDR_SIZE);

	if (!csr_lookup_pmkid_using_bssid(mac, session, &pmkid_cache, &index))
		return false;
	session_pmkid = &session->PmkidCacheInfo[index].PMKID[0];
	for (i = 0; i < pmkid_count; i++) {
		if (vos_mem_compare(pmkid + (i * CSR_RSN_PMKID_SIZE),
			session_pmkid, CSR_RSN_PMKID_SIZE))
			return true;
	}

	VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_DEBUG,
		  "PMKID in PmkidCacheInfo doesn't match with PMKIDs of peer");

	return false;
}

tANI_BOOLEAN csrGetRSNInformation( tHalHandle hHal, tCsrAuthList *pAuthType, eCsrEncryptionType enType, tCsrEncryptionList *pMCEncryption,
                                   tDot11fIERSN *pRSNIe,
                           tANI_U8 *UnicastCypher,
                           tANI_U8 *MulticastCypher,
                           tANI_U8 *AuthSuite,
                           tCsrRSNCapabilities *Capabilities,
                           eCsrAuthType *pNegotiatedAuthtype,
                           eCsrEncryptionType *pNegotiatedMCCipher )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tANI_BOOLEAN fAcceptableCyphers = FALSE;
    tANI_U8 cUnicastCyphers = 0;
    tANI_U8 cMulticastCyphers = 0;
    tANI_U8 cAuthSuites = 0, i;
    tANI_U8 Unicast[ CSR_RSN_OUI_SIZE ];
    tANI_U8 Multicast[ CSR_RSN_OUI_SIZE ];
    tANI_U8 AuthSuites[ CSR_RSN_MAX_AUTH_SUITES ][ CSR_RSN_OUI_SIZE ];
    tANI_U8 Authentication[ CSR_RSN_OUI_SIZE ];
    tANI_U8 MulticastCyphers[ CSR_RSN_MAX_MULTICAST_CYPHERS ][ CSR_RSN_OUI_SIZE ];
    eCsrAuthType negAuthType = eCSR_AUTH_TYPE_UNKNOWN;

    do{
        if ( pRSNIe->present )
        {
            cMulticastCyphers++;
            vos_mem_copy(MulticastCyphers, pRSNIe->gp_cipher_suite, CSR_RSN_OUI_SIZE);
            cUnicastCyphers = (tANI_U8)(pRSNIe->pwise_cipher_suite_count);
            cAuthSuites = (tANI_U8)(pRSNIe->akm_suite_cnt);
            for(i = 0; i < cAuthSuites && i < CSR_RSN_MAX_AUTH_SUITES; i++)
            {
                vos_mem_copy((void *)&AuthSuites[i],
                             (void *)&pRSNIe->akm_suite[i],
                             CSR_RSN_OUI_SIZE);
            }

            //Check - Is requested Unicast Cipher supported by the BSS.
            fAcceptableCyphers = csrMatchRSNOUIIndex( pMac, pRSNIe->pwise_cipher_suites, cUnicastCyphers,
                    csrGetOUIIndexFromCipher( enType ), Unicast );

            if( !fAcceptableCyphers ) break;


            //Unicast is supported. Pick the first matching Group cipher, if any.
            for( i = 0 ; i < pMCEncryption->numEntries ; i++ )
            {
                fAcceptableCyphers = csrMatchRSNOUIIndex( pMac, MulticastCyphers,  cMulticastCyphers,
                            csrGetOUIIndexFromCipher( pMCEncryption->encryptionType[i] ), Multicast );
                if(fAcceptableCyphers)
                {
                    break;
                }
            }
            if( !fAcceptableCyphers ) break;

            if( pNegotiatedMCCipher )
                *pNegotiatedMCCipher = pMCEncryption->encryptionType[i];

            /* Initializing with FALSE as it has TRUE value already */
            fAcceptableCyphers = FALSE;
            for (i = 0 ; i < pAuthType->numEntries; i++)
            {
                //Ciphers are supported, Match authentication algorithm and pick first matching authtype.

                 /* Set FILS as first preference */
                csr_is_fils_auth(pMac, AuthSuites, cAuthSuites,
                                 Authentication, pAuthType, i, &negAuthType);
                csr_check_sae_auth(pMac, AuthSuites, cAuthSuites,
                                 Authentication, pAuthType, i, &negAuthType);
#ifdef WLAN_FEATURE_VOWIFI_11R
                /* Changed the AKM suites according to order of preference */
                if ( (negAuthType == eCSR_AUTH_TYPE_UNKNOWN) && csrIsFTAuthRSN( pMac, AuthSuites, cAuthSuites, Authentication ) )
                {
                    if (eCSR_AUTH_TYPE_FT_RSN == pAuthType->authType[i])
                        negAuthType = eCSR_AUTH_TYPE_FT_RSN;
                }
                if ( (negAuthType == eCSR_AUTH_TYPE_UNKNOWN) && csrIsFTAuthRSNPsk( pMac, AuthSuites, cAuthSuites, Authentication ) )
                {
                    if (eCSR_AUTH_TYPE_FT_RSN_PSK == pAuthType->authType[i])
                        negAuthType = eCSR_AUTH_TYPE_FT_RSN_PSK;
                }
#endif
#ifdef FEATURE_WLAN_ESE
                /* ESE only supports 802.1X.  No PSK. */
                if ( (negAuthType == eCSR_AUTH_TYPE_UNKNOWN) && csrIsEseCckmAuthRSN( pMac, AuthSuites, cAuthSuites, Authentication ) )
                {
                    if (eCSR_AUTH_TYPE_CCKM_RSN == pAuthType->authType[i])
                        negAuthType = eCSR_AUTH_TYPE_CCKM_RSN;
                }
#endif
                if ( (negAuthType == eCSR_AUTH_TYPE_UNKNOWN) && csrIsAuthRSN( pMac, AuthSuites, cAuthSuites, Authentication ) )
                {
                    if (eCSR_AUTH_TYPE_RSN == pAuthType->authType[i])
                        negAuthType = eCSR_AUTH_TYPE_RSN;
                }
                if ((negAuthType == eCSR_AUTH_TYPE_UNKNOWN) && csrIsAuthRSNPsk( pMac, AuthSuites, cAuthSuites, Authentication ) )
                {
                    if (eCSR_AUTH_TYPE_RSN_PSK == pAuthType->authType[i])
                        negAuthType = eCSR_AUTH_TYPE_RSN_PSK;
                }
#ifdef WLAN_FEATURE_11W
                if ((negAuthType == eCSR_AUTH_TYPE_UNKNOWN) && csrIsAuthRSNPskSha256( pMac, AuthSuites, cAuthSuites, Authentication ) )
                {
                    if (eCSR_AUTH_TYPE_RSN_PSK_SHA256 == pAuthType->authType[i])
                        negAuthType = eCSR_AUTH_TYPE_RSN_PSK_SHA256;
                }
                if ((negAuthType == eCSR_AUTH_TYPE_UNKNOWN) &&
                    csrIsAuthRSN8021xSha256(pMac, AuthSuites,
                                             cAuthSuites, Authentication)) {
                    if (eCSR_AUTH_TYPE_RSN_8021X_SHA256 ==
                                                     pAuthType->authType[i])
                        negAuthType = eCSR_AUTH_TYPE_RSN_8021X_SHA256;
                }
#endif
                if ((negAuthType == eCSR_AUTH_TYPE_UNKNOWN) &&
                    csr_is_auth_wpa_owe(pMac, AuthSuites,
                                        cAuthSuites, Authentication)) {
                    if (eCSR_AUTH_TYPE_OWE == pAuthType->authType[i])
                        negAuthType = eCSR_AUTH_TYPE_OWE;
                }

                // The 1st auth type in the APs RSN IE, to match stations connecting
                // profiles auth type will cause us to exit this loop
                // This is added as some APs advertise multiple akms in the RSN IE.
                if (eCSR_AUTH_TYPE_UNKNOWN != negAuthType)
                {
                    fAcceptableCyphers = TRUE;
                    break;
                }
            } // for
        }

    }while (0);

    if ( fAcceptableCyphers )
    {
        if ( MulticastCypher )
        {
            vos_mem_copy(MulticastCypher, Multicast, CSR_RSN_OUI_SIZE);
        }

        if ( UnicastCypher )
        {
            vos_mem_copy(UnicastCypher, Unicast, CSR_RSN_OUI_SIZE);
        }

        if ( AuthSuite )
        {
            vos_mem_copy(AuthSuite, Authentication, CSR_RSN_OUI_SIZE);
        }

        if ( pNegotiatedAuthtype )
        {
            *pNegotiatedAuthtype = negAuthType;
        }
        if ( Capabilities )
        {
            Capabilities->PreAuthSupported = (pRSNIe->RSN_Cap[0] >> 0) & 0x1 ; // Bit 0 PreAuthentication
            Capabilities->NoPairwise = (pRSNIe->RSN_Cap[0] >> 1) & 0x1 ; // Bit 1 No Pairwise
            Capabilities->PTKSAReplayCounter = (pRSNIe->RSN_Cap[0] >> 2) & 0x3 ; // Bit 2, 3 PTKSA Replay Counter
            Capabilities->GTKSAReplayCounter = (pRSNIe->RSN_Cap[0] >> 4) & 0x3 ; // Bit 4, 5 GTKSA Replay Counter
#ifdef WLAN_FEATURE_11W
            Capabilities->MFPRequired = (pRSNIe->RSN_Cap[0] >> 6) & 0x1 ; // Bit 6 MFPR
            Capabilities->MFPCapable = (pRSNIe->RSN_Cap[0] >> 7) & 0x1 ; // Bit 7 MFPC
#else
            Capabilities->MFPRequired = 0 ; // Bit 6 MFPR
            Capabilities->MFPCapable = 0 ; // Bit 7 MFPC
#endif
            Capabilities->Reserved = pRSNIe->RSN_Cap[1]  & 0xff ; // remaining reserved
        }
    }
    return( fAcceptableCyphers );
}

#ifdef WLAN_FEATURE_11W
/* ---------------------------------------------------------------------------
    \fn csrIsPMFCapabilitiesInRSNMatch

    \brief this function is to match our current capabilities with the AP
           to which we are expecting make the connection.

    \param hHal               - HAL Pointer
           pFilterMFPEnabled  - given by supplicant to us to specify what kind
                                of connection supplicant is expecting to make
                                if it is enabled then make PMF connection.
                                if it is disabled then make normal connection.
           pFilterMFPRequired - given by supplicant based on our configuration
                                if it is 1 then we will require mandatory
                                PMF connection and if it is 0 then we PMF
                                connection is optional.
           pFilterMFPCapable  - given by supplicant based on our configuration
                                if it 1 then we are PMF capable and if it 0
                                then we are not PMF capable.
           pRSNIe             - RSNIe from Beacon/probe response of
                                neighbor AP against which we will compare
                                our capabilities.

    \return tANI_BOOLEAN      - if our PMF capabilities matches with AP then we
                                will return true to indicate that we are good
                                to make connection with it. Else we will return
                                false.
  -------------------------------------------------------------------------------*/
static tANI_BOOLEAN
csrIsPMFCapabilitiesInRSNMatch( tHalHandle hHal,
                                tANI_BOOLEAN *pFilterMFPEnabled,
                                tANI_U8 *pFilterMFPRequired,
                                tANI_U8 *pFilterMFPCapable,
                                tDot11fIERSN *pRSNIe)
{
    tANI_U8 apProfileMFPCapable  = 0;
    tANI_U8 apProfileMFPRequired = 0;
    if (pRSNIe && pFilterMFPEnabled && pFilterMFPCapable && pFilterMFPRequired)
    {
       /* Extracting MFPCapable bit from RSN Ie */
       apProfileMFPCapable  = (pRSNIe->RSN_Cap[0] >> 7) & 0x1;
       apProfileMFPRequired = (pRSNIe->RSN_Cap[0] >> 6) & 0x1;

       VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_INFO,
          FL("pFilterMFPEnabled=%d pFilterMFPRequired=%d pFilterMFPCapable=%d apProfileMFPCapable=%d apProfileMFPRequired=%d"),
               *pFilterMFPEnabled, *pFilterMFPRequired, *pFilterMFPCapable,
               apProfileMFPCapable, apProfileMFPRequired);

       if (*pFilterMFPEnabled && *pFilterMFPCapable && *pFilterMFPRequired
           && (apProfileMFPCapable == 0))
       {
           VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_INFO,
                     "AP is not capable to make PMF connection");
           return VOS_FALSE;
       }
       else if (!(*pFilterMFPCapable) &&
                apProfileMFPCapable && apProfileMFPRequired)
       {

           /*
            * In this case, AP with whom we trying to connect requires
            * mandatory PMF connections and we are not capable so this AP
            * is not good choice to connect
            */
           VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_INFO,
           "AP needs PMF connection and we are not capable of pmf connection");
           return VOS_FALSE;
       }
    }
    return VOS_TRUE;
}
#endif

tANI_BOOLEAN csrIsRSNMatch( tHalHandle hHal, tCsrAuthList *pAuthType,
                            eCsrEncryptionType enType,
                            tCsrEncryptionList *pEnMcType,
                            tANI_BOOLEAN *pMFPEnabled, tANI_U8 *pMFPRequired,
                            tANI_U8 *pMFPCapable,
                            tDot11fBeaconIEs *pIes,
                            eCsrAuthType *pNegotiatedAuthType,
                            eCsrEncryptionType *pNegotiatedMCCipher )
{
    tANI_BOOLEAN fRSNMatch = FALSE;

        // See if the cyphers in the Bss description match with the settings in the profile.
    fRSNMatch = csrGetRSNInformation( hHal, pAuthType, enType, pEnMcType, &pIes->RSN, NULL, NULL, NULL, NULL,
                                      pNegotiatedAuthType, pNegotiatedMCCipher );
#ifdef WLAN_FEATURE_11W
    /* If all the filter matches then finally checks for PMF capabilities */
    if (fRSNMatch)
    {
        fRSNMatch = csrIsPMFCapabilitiesInRSNMatch( hHal, pMFPEnabled,
                                                    pMFPRequired, pMFPCapable,
                                                    &pIes->RSN);
    }
#endif
    return( fRSNMatch );
}

/**
 * csr_lookup_pmkid_using_ssid() - lookup pmkid using ssid and cache_id
 * @mac: pointer to mac
 * @session: sme session pointer
 * @pmk_cache: pointer to pmk cache
 * @index: index value needs to be seached
 *
 * Return: true if pmkid is found else false
 */
static bool csr_lookup_pmkid_using_ssid(tpAniSirGlobal mac,
                    tCsrRoamSession *session,
                    tPmkidCacheInfo *pmk_cache,
                    uint32_t *index)
{
    uint32_t i;
    tPmkidCacheInfo *session_pmk;

    for (i = 0; i < session->NumPmkidCache; i++) {
        session_pmk = &session->PmkidCacheInfo[i];

        if ((!adf_os_mem_cmp(pmk_cache->ssid, session_pmk->ssid,
                  pmk_cache->ssid_len)) &&
            (!adf_os_mem_cmp(session_pmk->cache_id,
                  pmk_cache->cache_id, CACHE_ID_LEN))) {
            /* match found */
            *index = i;
            return true;
        }
    }

    return false;
}

bool csr_lookup_pmkid_using_bssid(tpAniSirGlobal mac,
                    tCsrRoamSession *session,
                    tPmkidCacheInfo *pmk_cache,
                    uint32_t *index)
{
    uint32_t i;
    tPmkidCacheInfo *session_pmk;

    for (i = 0; i < session->NumPmkidCache; i++) {
        session_pmk = &session->PmkidCacheInfo[i];
        if (vos_is_macaddr_equal((v_MACADDR_t *)pmk_cache->BSSID,
                     (v_MACADDR_t *)session_pmk->BSSID)) {
            /* match found */
            *index = i;
            return true;
        }
    }

    return false;
}

tANI_BOOLEAN csrLookupPMKID( tpAniSirGlobal pMac, tANI_U32 sessionId, tPmkidCacheInfo *pmk_cache)
{
    tANI_BOOLEAN fRC = FALSE, fMatchFound = FALSE;
    tANI_U32 Index;
    tCsrRoamSession *pSession = CSR_GET_SESSION( pMac, sessionId );

    if(!pSession)
    {
        smsLog(pMac, LOGE, FL("  session %d not found "), sessionId);
        return FALSE;
    }

    if (pmk_cache->ssid_len) {
        /* Try to find based on cache_id and ssid first */
        fMatchFound = csr_lookup_pmkid_using_ssid(pMac, pSession,
                                                  pmk_cache, &Index);
    }

    /* If not able to find using cache id or ssid_len is not present */
    if (!fMatchFound)
        fMatchFound = csr_lookup_pmkid_using_bssid(pMac,
                                        pSession, pmk_cache, &Index);

    if (!fMatchFound) {
        smsLog(pMac, LOG2, "No PMKID Match Found");
        return false;
    }

    vos_mem_copy(pmk_cache->PMKID, pSession->PmkidCacheInfo[Index].PMKID, CSR_RSN_PMKID_SIZE);
    vos_mem_copy(pmk_cache->pmk,
                 pSession->PmkidCacheInfo[Index].pmk,
                 pSession->PmkidCacheInfo[Index].pmk_len);
    pmk_cache->pmk_len = pSession->PmkidCacheInfo[Index].pmk_len;

    fRC = TRUE;

    smsLog(pMac, LOG1, "csrLookupPMKID called return match = %d pMac->roam.NumPmkidCache = %d",
        fRC, pSession->NumPmkidCache);

    return fRC;
}

#ifdef WLAN_FEATURE_FILS_SK
/*
 * csr_update_pmksa_for_cache_id: update tPmkidCacheInfo to lookup using
 * ssid and cache id
 * @bss_desc: bss description
 * @profile: csr roam profile
 * @pmkid_cache: pmksa cache
 *
 * Return: true if cache identifier present else false
 */
static bool csr_update_pmksa_for_cache_id(tSirBssDescription *bss_desc,
                tCsrRoamProfile *profile,
                tPmkidCacheInfo *pmkid_cache)
{
    if (!bss_desc->fils_info_element.is_cache_id_present)
        return false;

    pmkid_cache->ssid_len =
        profile->SSIDs.SSIDList[0].SSID.length;
    vos_mem_copy(pmkid_cache->ssid,
        profile->SSIDs.SSIDList[0].SSID.ssId,
        profile->SSIDs.SSIDList[0].SSID.length);
    vos_mem_copy(pmkid_cache->cache_id,
        bss_desc->fils_info_element.cache_id,
        CACHE_ID_LEN);
    vos_mem_copy(pmkid_cache->BSSID,
        bss_desc->bssId, VOS_MAC_ADDR_SIZE);

    return true;
}

/*
 * csr_update_pmksa_to_profile: update pmk and pmkid to profile which will be
 * used in case of fils session
 * @profile: profile
 * @pmkid_cache: pmksa cache
 *
 * Return: None
 */
static inline void csr_update_pmksa_to_profile(tCsrRoamProfile *profile,
        tPmkidCacheInfo *pmkid_cache)
{
    if (!profile->fils_con_info)
        return;

    profile->fils_con_info->pmk_len = pmkid_cache->pmk_len;
    vos_mem_copy(profile->fils_con_info->pmk,
            pmkid_cache->pmk, pmkid_cache->pmk_len);
    vos_mem_copy(profile->fils_con_info->pmkid,
        pmkid_cache->PMKID, CSR_RSN_PMKID_SIZE);

}
#else
static inline bool csr_update_pmksa_for_cache_id(tSirBssDescription *bss_desc,
                tCsrRoamProfile *profile,
                tPmkidCacheInfo *pmkid_cache)
{
    return false;
}

static inline void csr_update_pmksa_to_profile(tCsrRoamProfile *profile,
        tPmkidCacheInfo *pmkid_cache)
{
}
#endif

tANI_U8 csrConstructRSNIe( tHalHandle hHal, tANI_U32 sessionId, tCsrRoamProfile *pProfile,
                            tSirBssDescription *pSirBssDesc, tDot11fBeaconIEs *pIes, tCsrRSNIe *pRSNIe )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tANI_BOOLEAN fRSNMatch;
    tANI_U8 cbRSNIe = 0;
    tANI_U8 UnicastCypher[ CSR_RSN_OUI_SIZE ];
    tANI_U8 MulticastCypher[ CSR_RSN_OUI_SIZE ];
    tANI_U8 AuthSuite[ CSR_RSN_OUI_SIZE ];
    tCsrRSNAuthIe *pAuthSuite;
    tCsrRSNCapabilities RSNCapabilities;
    tCsrRSNPMKIe        *pPMK;
    tPmkidCacheInfo pmkid_cache;
#ifdef WLAN_FEATURE_11W
    tANI_U8 *pGroupMgmtCipherSuite;
#endif
    tDot11fBeaconIEs *pIesLocal = pIes;
    eCsrAuthType negAuthType = eCSR_AUTH_TYPE_UNKNOWN;
    tDot11fIERSN dot11RSNIE;
    tANI_U32 status;
    smsLog(pMac, LOGW, "%s called...", __func__);

    do
    {
        if ( !csrIsProfileRSN( pProfile ) ) break;

        if( !pIesLocal && (!HAL_STATUS_SUCCESS(csrGetParsedBssDescriptionIEs(pMac, pSirBssDesc, &pIesLocal))) )
        {
            break;
        }

        memset(&dot11RSNIE, 0, sizeof(tDot11fIERSN));
        /*
         *  Use intersection of the RSN cap sent by user space and
         *  the AP, so that only common capability are enabled.
         */
        if(pProfile->nRSNReqIELength && pProfile->pRSNReqIE) {
            status = dot11fUnpackIeRSN(hHal, pProfile->pRSNReqIE + 2,
                                       pProfile->nRSNReqIELength - 2, &dot11RSNIE);
            if (DOT11F_SUCCEEDED(status)) {
                pIesLocal->RSN.RSN_Cap[0] =
                        pIesLocal->RSN.RSN_Cap[0] &
                        dot11RSNIE.RSN_Cap[0];
                pIesLocal->RSN.RSN_Cap[1] =
                        pIesLocal->RSN.RSN_Cap[1] &
                        dot11RSNIE.RSN_Cap[1];
            }
        }

        // See if the cyphers in the Bss description match with the settings in the profile.
        fRSNMatch = csrGetRSNInformation( hHal, &pProfile->AuthType, pProfile->negotiatedUCEncryptionType,
                                            &pProfile->mcEncryptionType, &pIesLocal->RSN,
                                            UnicastCypher, MulticastCypher, AuthSuite, &RSNCapabilities, &negAuthType, NULL );
        if ( !fRSNMatch ) break;

        pRSNIe->IeHeader.ElementID = SIR_MAC_RSN_EID;

        pRSNIe->Version = CSR_RSN_VERSION_SUPPORTED;

        vos_mem_copy(pRSNIe->MulticastOui, MulticastCypher, sizeof( MulticastCypher ));

        pRSNIe->cUnicastCyphers = 1;

        vos_mem_copy(&pRSNIe->UnicastOui[ 0 ], UnicastCypher, sizeof( UnicastCypher ));

        pAuthSuite = (tCsrRSNAuthIe *)( &pRSNIe->UnicastOui[ pRSNIe->cUnicastCyphers ] );

        pAuthSuite->cAuthenticationSuites = 1;
        vos_mem_copy(&pAuthSuite->AuthOui[ 0 ], AuthSuite, sizeof( AuthSuite ));

        /*
         * RSN capabilities follows the Auth Suite (two octets)
         * !!REVIEW - What should STA put in RSN capabilities, currently
         * just putting back APs capabilities For one, we shouldn't EVER be
         * sending out "pre-auth supported". It is an AP only capability.
         * For another, we should use the Management Frame Protection
         * values given by the supplicant
         */
        RSNCapabilities.PreAuthSupported = 0;
#ifdef WLAN_FEATURE_11W
        if (RSNCapabilities.MFPCapable && pProfile->MFPCapable) {
            RSNCapabilities.MFPCapable = pProfile->MFPCapable;
            RSNCapabilities.MFPRequired = pProfile->MFPRequired;
        }
        else {
            RSNCapabilities.MFPCapable = 0;
            RSNCapabilities.MFPRequired = 0;
        }
#endif
        *(tANI_U16 *)( &pAuthSuite->AuthOui[ 1 ] ) = *((tANI_U16 *)(&RSNCapabilities));

        pPMK = (tCsrRSNPMKIe *)( ((tANI_U8 *)(&pAuthSuite->AuthOui[ 1 ])) + sizeof(tANI_U16) );
        if (!csr_update_pmksa_for_cache_id(pSirBssDesc, pProfile, &pmkid_cache))
            vos_mem_copy((v_MACADDR_t *)pmkid_cache.BSSID,
                        (v_MACADDR_t *)pSirBssDesc->bssId,
                         VOS_MAC_ADDR_SIZE);
        // Don't include the PMK SA IDs for CCKM associations.
        if (
#ifdef FEATURE_WLAN_ESE
                (eCSR_AUTH_TYPE_CCKM_RSN != negAuthType) &&
#endif
              csrLookupPMKID( pMac, sessionId, &pmkid_cache))
        {
            pPMK->cPMKIDs = 1;

            vos_mem_copy(pPMK->PMKIDList[0].PMKID, 
                         pmkid_cache.PMKID,
                         CSR_RSN_PMKID_SIZE);
            csr_update_pmksa_to_profile(pProfile, &pmkid_cache);
        }
        else
        {
            pPMK->cPMKIDs = 0;
        }

#ifdef WLAN_FEATURE_11W
         /* Advertise BIP in group cipher key management only if PMF is enabled
          * and AP is capable.
          */
        if (pProfile->MFPEnabled &&
                (RSNCapabilities.MFPCapable && pProfile->MFPCapable)){
            pGroupMgmtCipherSuite = (tANI_U8 *) pPMK + sizeof ( tANI_U16 ) +
                ( pPMK->cPMKIDs * CSR_RSN_PMKID_SIZE );
            vos_mem_copy(pGroupMgmtCipherSuite, csrRSNOui[07], CSR_WPA_OUI_SIZE);
        }
#endif

        // Add in the fixed fields plus 1 Unicast cypher, less the IE Header length
        // Add in the size of the Auth suite (count plus a single OUI)
        // Add in the RSN caps field.
        // Add PMKID count and PMKID (if any)
        // Add group management cipher suite
        pRSNIe->IeHeader.Length = (tANI_U8) (sizeof( *pRSNIe ) - sizeof ( pRSNIe->IeHeader ) +
                                  sizeof( *pAuthSuite ) +
                                  sizeof( tCsrRSNCapabilities ));
        if(pPMK->cPMKIDs)
        {
            pRSNIe->IeHeader.Length += (tANI_U8)(sizeof( tANI_U16 ) +
                                        (pPMK->cPMKIDs * CSR_RSN_PMKID_SIZE));
        }
#ifdef WLAN_FEATURE_11W
        if (pProfile->MFPEnabled &&
                (RSNCapabilities.MFPCapable && pProfile->MFPCapable)){
            if ( 0 == pPMK->cPMKIDs )
                pRSNIe->IeHeader.Length += sizeof( tANI_U16 );
            pRSNIe->IeHeader.Length += CSR_WPA_OUI_SIZE;
        }
#endif

        // return the size of the IE header (total) constructed...
        cbRSNIe = pRSNIe->IeHeader.Length + sizeof( pRSNIe->IeHeader );

    } while( 0 );

    if( !pIes && pIesLocal )
    {
        //locally allocated
        vos_mem_free(pIesLocal);
    }

    return( cbRSNIe );
}


#ifdef FEATURE_WLAN_WAPI
tANI_BOOLEAN csrGetWapiInformation( tHalHandle hHal, tCsrAuthList *pAuthType, eCsrEncryptionType enType, tCsrEncryptionList *pMCEncryption,
                                   tDot11fIEWAPI *pWapiIe,
                                    tANI_U8 *UnicastCypher,
                                    tANI_U8 *MulticastCypher,
                                    tANI_U8 *AuthSuite,
                                    eCsrAuthType *pNegotiatedAuthtype,
                                    eCsrEncryptionType *pNegotiatedMCCipher )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tANI_BOOLEAN fAcceptableCyphers = FALSE;
    tANI_U8 cUnicastCyphers = 0;
    tANI_U8 cMulticastCyphers = 0;
    tANI_U8 cAuthSuites = 0, i;
    tANI_U8 Unicast[ CSR_WAPI_OUI_SIZE ];
    tANI_U8 Multicast[ CSR_WAPI_OUI_SIZE ];
    tANI_U8 AuthSuites[ CSR_WAPI_MAX_AUTH_SUITES ][ CSR_WAPI_OUI_SIZE ];
    tANI_U8 Authentication[ CSR_WAPI_OUI_SIZE ];
    tANI_U8 MulticastCyphers[ CSR_WAPI_MAX_MULTICAST_CYPHERS ][ CSR_WAPI_OUI_SIZE ];
    eCsrAuthType negAuthType = eCSR_AUTH_TYPE_UNKNOWN;
    tANI_U8 wapiOuiIndex = 0;
    do{
        if ( pWapiIe->present )
        {
            cMulticastCyphers++;
            vos_mem_copy(MulticastCyphers, pWapiIe->multicast_cipher_suite,
                         CSR_WAPI_OUI_SIZE);
            cUnicastCyphers = (tANI_U8)(pWapiIe->unicast_cipher_suite_count);
            cAuthSuites = (tANI_U8)(pWapiIe->akm_suite_count);
            for(i = 0; i < cAuthSuites && i < CSR_WAPI_MAX_AUTH_SUITES; i++)
            {
                vos_mem_copy((void *)&AuthSuites[i], (void *)&pWapiIe->akm_suites[i],
                             CSR_WAPI_OUI_SIZE);
            }

            wapiOuiIndex = csrGetOUIIndexFromCipher( enType );
            if (wapiOuiIndex >= CSR_OUI_WAPI_WAI_MAX_INDEX)
            {
                smsLog(pMac, LOGE, FL("Wapi OUI index = %d out of limit"), wapiOuiIndex);
                fAcceptableCyphers = FALSE;
                break;
            }
            //Check - Is requested Unicast Cipher supported by the BSS.
            fAcceptableCyphers = csrMatchWapiOUIIndex( pMac, pWapiIe->unicast_cipher_suites, cUnicastCyphers,
                    wapiOuiIndex, Unicast );

            if( !fAcceptableCyphers ) break;

            //Unicast is supported. Pick the first matching Group cipher, if any.
            for( i = 0 ; i < pMCEncryption->numEntries ; i++ )
            {
                wapiOuiIndex = csrGetOUIIndexFromCipher( pMCEncryption->encryptionType[i] );
                if (wapiOuiIndex >= CSR_OUI_WAPI_WAI_MAX_INDEX)
                {
                    smsLog(pMac, LOGE, FL("Wapi OUI index = %d out of limit"), wapiOuiIndex);
                    fAcceptableCyphers = FALSE;
                    break;
                }
                fAcceptableCyphers = csrMatchWapiOUIIndex( pMac, MulticastCyphers,  cMulticastCyphers,
                        wapiOuiIndex, Multicast );
                if(fAcceptableCyphers)
                {
                    break;
                }
            }
            if( !fAcceptableCyphers ) break;

            if( pNegotiatedMCCipher )
                *pNegotiatedMCCipher = pMCEncryption->encryptionType[i];

            /* Ciphers are supported, Match authentication algorithm and pick
               first matching auth type. */
            if ( csrIsAuthWapiCert( pMac, AuthSuites, cAuthSuites, Authentication ) )
            {
                negAuthType = eCSR_AUTH_TYPE_WAPI_WAI_CERTIFICATE;
            }
            else if ( csrIsAuthWapiPsk( pMac, AuthSuites, cAuthSuites, Authentication ) )
            {
                negAuthType = eCSR_AUTH_TYPE_WAPI_WAI_PSK;
            }
            else
            {
                fAcceptableCyphers = FALSE;
                negAuthType = eCSR_AUTH_TYPE_UNKNOWN;
            }
            if( ( 0 == pAuthType->numEntries ) || ( FALSE == fAcceptableCyphers ) )
            {
                //Caller doesn't care about auth type, or BSS doesn't match
                break;
            }
            fAcceptableCyphers = FALSE;
            for( i = 0 ; i < pAuthType->numEntries; i++ )
            {
                if( pAuthType->authType[i] == negAuthType )
                {
                    fAcceptableCyphers = TRUE;
                    break;
                }
            }
        }
    }while (0);

    if ( fAcceptableCyphers )
    {
        if ( MulticastCypher )
        {
           vos_mem_copy(MulticastCypher, Multicast, CSR_WAPI_OUI_SIZE);
        }

        if ( UnicastCypher )
        {
            vos_mem_copy(UnicastCypher, Unicast, CSR_WAPI_OUI_SIZE);
        }

        if ( AuthSuite )
        {
            vos_mem_copy(AuthSuite, Authentication, CSR_WAPI_OUI_SIZE);
        }

        if ( pNegotiatedAuthtype )
        {
            *pNegotiatedAuthtype = negAuthType;
        }
    }
    return( fAcceptableCyphers );
}

tANI_BOOLEAN csrIsWapiMatch( tHalHandle hHal, tCsrAuthList *pAuthType, eCsrEncryptionType enType, tCsrEncryptionList *pEnMcType,
                            tDot11fBeaconIEs *pIes, eCsrAuthType *pNegotiatedAuthType, eCsrEncryptionType *pNegotiatedMCCipher )
{
    tANI_BOOLEAN fWapiMatch = FALSE;

        // See if the cyphers in the Bss description match with the settings in the profile.
    fWapiMatch = csrGetWapiInformation( hHal, pAuthType, enType, pEnMcType, &pIes->WAPI, NULL, NULL, NULL,
                                      pNegotiatedAuthType, pNegotiatedMCCipher );

    return( fWapiMatch );
}

tANI_BOOLEAN csrLookupBKID( tpAniSirGlobal pMac, tANI_U32 sessionId, tANI_U8 *pBSSId, tANI_U8 *pBKId )
{
    tANI_BOOLEAN fRC = FALSE, fMatchFound = FALSE;
    tANI_U32 Index;
    tCsrRoamSession *pSession = CSR_GET_SESSION( pMac, sessionId );

    if(!pSession)
    {
        smsLog(pMac, LOGE, FL("  session %d not found "), sessionId);
        return FALSE;
    }

    do
    {
        for( Index=0; Index < pSession->NumBkidCache; Index++ )
        {
            smsLog(pMac, LOGW, "match BKID "MAC_ADDRESS_STR" to ",
                   MAC_ADDR_ARRAY(pBSSId));
            if (vos_mem_compare(pBSSId, pSession->BkidCacheInfo[Index].BSSID, sizeof(tCsrBssid) ) )
            {
                // match found
                fMatchFound = TRUE;
                break;
            }
        }

        if( !fMatchFound ) break;

        vos_mem_copy(pBKId, pSession->BkidCacheInfo[Index].BKID, CSR_WAPI_BKID_SIZE);

        fRC = TRUE;
    }
    while( 0 );
    smsLog(pMac, LOGW, "csrLookupBKID called return match = %d pMac->roam.NumBkidCache = %d", fRC, pSession->NumBkidCache);

    return fRC;
}

tANI_U8 csrConstructWapiIe( tpAniSirGlobal pMac, tANI_U32 sessionId, tCsrRoamProfile *pProfile,
                            tSirBssDescription *pSirBssDesc, tDot11fBeaconIEs *pIes, tCsrWapiIe *pWapiIe )
{
    tANI_BOOLEAN fWapiMatch = FALSE;
    tANI_U8 cbWapiIe = 0;
    tANI_U8 UnicastCypher[ CSR_WAPI_OUI_SIZE ];
    tANI_U8 MulticastCypher[ CSR_WAPI_OUI_SIZE ];
    tANI_U8 AuthSuite[ CSR_WAPI_OUI_SIZE ];
    tANI_U8 BKId[CSR_WAPI_BKID_SIZE];
    tANI_U8 *pWapi = NULL;
    tANI_BOOLEAN fBKIDFound = FALSE;
    tDot11fBeaconIEs *pIesLocal = pIes;

    do
    {
        if ( !csrIsProfileWapi( pProfile ) ) break;

        if( !pIesLocal && (!HAL_STATUS_SUCCESS(csrGetParsedBssDescriptionIEs(pMac, pSirBssDesc, &pIesLocal))) )
        {
            break;
        }

        // See if the cyphers in the Bss description match with the settings in the profile.
        fWapiMatch = csrGetWapiInformation( pMac, &pProfile->AuthType, pProfile->negotiatedUCEncryptionType,
                                            &pProfile->mcEncryptionType, &pIesLocal->WAPI,
                                            UnicastCypher, MulticastCypher, AuthSuite, NULL, NULL );
        if ( !fWapiMatch ) break;

        vos_mem_set(pWapiIe, sizeof(tCsrWapiIe), 0);

        pWapiIe->IeHeader.ElementID = DOT11F_EID_WAPI;

        pWapiIe->Version = CSR_WAPI_VERSION_SUPPORTED;

        pWapiIe->cAuthenticationSuites = 1;
        vos_mem_copy(&pWapiIe->AuthOui[ 0 ], AuthSuite, sizeof( AuthSuite ));

        pWapi = (tANI_U8 *) (&pWapiIe->AuthOui[ 1 ]);

        *pWapi = (tANI_U16)1; //cUnicastCyphers
        pWapi+=2;
        vos_mem_copy(pWapi, UnicastCypher, sizeof( UnicastCypher ));
        pWapi += sizeof( UnicastCypher );

        vos_mem_copy(pWapi, MulticastCypher, sizeof( MulticastCypher ));
        pWapi += sizeof( MulticastCypher );


        /*
         * WAPI capabilities follows the Auth Suite (two octets)
         * we shouldn't EVER be sending out "pre-auth supported".
         * It is an AP only capability & since we already did a memset
         * pWapiIe to 0, skip these fields
         */
        pWapi +=2;

        fBKIDFound = csrLookupBKID( pMac, sessionId, pSirBssDesc->bssId, &(BKId[0]) );


        if( fBKIDFound )
        {
            /* Do we need to change the endianness here */
            *pWapi = (tANI_U16)1; //cBKIDs
            pWapi+=2;
            vos_mem_copy(pWapi, BKId, CSR_WAPI_BKID_SIZE);
        }
        else
        {
            *pWapi = 0;
            pWapi+=1;
            *pWapi = 0;
            pWapi+=1;
        }

        // Add in the IE fields except the IE header
        // Add BKID count and BKID (if any)
        pWapiIe->IeHeader.Length = (tANI_U8) (sizeof( *pWapiIe ) - sizeof ( pWapiIe->IeHeader ));

        /*2 bytes for BKID Count field*/
        pWapiIe->IeHeader.Length += sizeof( tANI_U16 );

        if(fBKIDFound)
        {
            pWapiIe->IeHeader.Length += CSR_WAPI_BKID_SIZE;
        }
        // return the size of the IE header (total) constructed...
        cbWapiIe = pWapiIe->IeHeader.Length + sizeof( pWapiIe->IeHeader );

    } while( 0 );

    if( !pIes && pIesLocal )
    {
        //locally allocated
        vos_mem_free(pIesLocal);
    }

    return( cbWapiIe );
}
#endif /* FEATURE_WLAN_WAPI */

tANI_BOOLEAN csrGetWpaCyphers( tpAniSirGlobal pMac, tCsrAuthList *pAuthType, eCsrEncryptionType enType, tCsrEncryptionList *pMCEncryption,
                               tDot11fIEWPA *pWpaIe,
                           tANI_U8 *UnicastCypher,
                           tANI_U8 *MulticastCypher,
                           tANI_U8 *AuthSuite,
                           eCsrAuthType *pNegotiatedAuthtype,
                           eCsrEncryptionType *pNegotiatedMCCipher )
{
    tANI_BOOLEAN fAcceptableCyphers = FALSE;
    tANI_U8 cUnicastCyphers = 0;
    tANI_U8 cMulticastCyphers = 0;
    tANI_U8 cAuthSuites = 0;
    tANI_U8 Unicast[ CSR_WPA_OUI_SIZE ];
    tANI_U8 Multicast[ CSR_WPA_OUI_SIZE ];
    tANI_U8 Authentication[ CSR_WPA_OUI_SIZE ];
    tANI_U8 MulticastCyphers[ 1 ][ CSR_WPA_OUI_SIZE ];
    tANI_U8 i;
    eCsrAuthType negAuthType = eCSR_AUTH_TYPE_UNKNOWN;

    do
    {
        if ( pWpaIe->present )
        {
            cMulticastCyphers = 1;
            vos_mem_copy(MulticastCyphers, pWpaIe->multicast_cipher, CSR_WPA_OUI_SIZE);
            cUnicastCyphers = (tANI_U8)(pWpaIe->unicast_cipher_count);
            cAuthSuites = (tANI_U8)(pWpaIe->auth_suite_count);

            //Check - Is requested Unicast Cipher supported by the BSS.
            fAcceptableCyphers = csrMatchWPAOUIIndex( pMac, pWpaIe->unicast_ciphers, cUnicastCyphers,
                    csrGetOUIIndexFromCipher( enType ), Unicast );

            if( !fAcceptableCyphers ) break;


            //Unicast is supported. Pick the first matching Group cipher, if any.
            for( i = 0 ; i < pMCEncryption->numEntries ; i++ )
            {
                fAcceptableCyphers = csrMatchWPAOUIIndex( pMac, MulticastCyphers,  cMulticastCyphers,
                            csrGetOUIIndexFromCipher( pMCEncryption->encryptionType[i]), Multicast );
                if(fAcceptableCyphers)
                {
                    break;
                }
            }
            if( !fAcceptableCyphers ) break;

            if( pNegotiatedMCCipher )
                *pNegotiatedMCCipher = pMCEncryption->encryptionType[i];

                /* Initializing with FALSE as it has TRUE value already */
            fAcceptableCyphers = FALSE;
            for (i = 0 ; i < pAuthType->numEntries; i++)
            {
                /* Ciphers are supported, Match authentication algorithm
                   and pick first matching auth type */
                if ( csrIsAuthWpa( pMac, pWpaIe->auth_suites, cAuthSuites, Authentication ) )
                {
                    if (eCSR_AUTH_TYPE_WPA == pAuthType->authType[i])
                    negAuthType = eCSR_AUTH_TYPE_WPA;
                }
                if ( (negAuthType == eCSR_AUTH_TYPE_UNKNOWN) && csrIsAuthWpaPsk( pMac, pWpaIe->auth_suites, cAuthSuites, Authentication ) )
                {
                    if (eCSR_AUTH_TYPE_WPA_PSK == pAuthType->authType[i])
                    negAuthType = eCSR_AUTH_TYPE_WPA_PSK;
                }
#ifdef FEATURE_WLAN_ESE
                if ( (negAuthType == eCSR_AUTH_TYPE_UNKNOWN) && csrIsEseCckmAuthWpa( pMac, pWpaIe->auth_suites, cAuthSuites, Authentication ) )
                {
                    if (eCSR_AUTH_TYPE_CCKM_WPA == pAuthType->authType[i])
                        negAuthType = eCSR_AUTH_TYPE_CCKM_WPA;
                }
#endif /* FEATURE_WLAN_ESE */

                // The 1st auth type in the APs WPA IE, to match stations connecting
                // profiles auth type will cause us to exit this loop
                // This is added as some APs advertise multiple akms in the WPA IE.
                if (eCSR_AUTH_TYPE_UNKNOWN != negAuthType)
                {
                        fAcceptableCyphers = TRUE;
                        break;
                    }
            } // for
            }
    }while(0);

    if ( fAcceptableCyphers )
    {
        if ( MulticastCypher )
        {
            vos_mem_copy((tANI_U8 **)MulticastCypher, Multicast, CSR_WPA_OUI_SIZE);
        }

        if ( UnicastCypher )
        {
            vos_mem_copy((tANI_U8 **)UnicastCypher, Unicast, CSR_WPA_OUI_SIZE);
        }

        if ( AuthSuite )
        {
            vos_mem_copy((tANI_U8 **)AuthSuite, Authentication, CSR_WPA_OUI_SIZE);
        }

        if( pNegotiatedAuthtype )
        {
            *pNegotiatedAuthtype = negAuthType;
        }
    }

    return( fAcceptableCyphers );
}



tANI_BOOLEAN csrIsWpaEncryptionMatch( tpAniSirGlobal pMac, tCsrAuthList *pAuthType, eCsrEncryptionType enType, tCsrEncryptionList *pEnMcType,
                                        tDot11fBeaconIEs *pIes, eCsrAuthType *pNegotiatedAuthtype, eCsrEncryptionType *pNegotiatedMCCipher )
{
    tANI_BOOLEAN fWpaMatch = eANI_BOOLEAN_FALSE;

        // See if the cyphers in the Bss description match with the settings in the profile.
    fWpaMatch = csrGetWpaCyphers( pMac, pAuthType, enType, pEnMcType, &pIes->WPA, NULL, NULL, NULL, pNegotiatedAuthtype, pNegotiatedMCCipher );

    return( fWpaMatch );
}


tANI_U8 csrConstructWpaIe( tHalHandle hHal, tCsrRoamProfile *pProfile, tSirBssDescription *pSirBssDesc,
                           tDot11fBeaconIEs *pIes, tCsrWpaIe *pWpaIe )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tANI_BOOLEAN fWpaMatch;
    tANI_U8 cbWpaIe = 0;
    tANI_U8 UnicastCypher[ CSR_WPA_OUI_SIZE ];
    tANI_U8 MulticastCypher[ CSR_WPA_OUI_SIZE ];
    tANI_U8 AuthSuite[ CSR_WPA_OUI_SIZE ];
    tCsrWpaAuthIe *pAuthSuite;
    tDot11fBeaconIEs *pIesLocal = pIes;

    do
    {
        if ( !csrIsProfileWpa( pProfile ) ) break;

        if( !pIesLocal && (!HAL_STATUS_SUCCESS(csrGetParsedBssDescriptionIEs(pMac, pSirBssDesc, &pIesLocal))) )
        {
            break;
        }
        // See if the cyphers in the Bss description match with the settings in the profile.
        fWpaMatch = csrGetWpaCyphers( hHal, &pProfile->AuthType, pProfile->negotiatedUCEncryptionType, &pProfile->mcEncryptionType,
                                      &pIesLocal->WPA, UnicastCypher, MulticastCypher, AuthSuite, NULL, NULL );
        if ( !fWpaMatch ) break;

        pWpaIe->IeHeader.ElementID = SIR_MAC_WPA_EID;

        vos_mem_copy(pWpaIe->Oui, csrWpaOui[01], sizeof( pWpaIe->Oui ));

        pWpaIe->Version = CSR_WPA_VERSION_SUPPORTED;

        vos_mem_copy(pWpaIe->MulticastOui, MulticastCypher, sizeof( MulticastCypher ));

        pWpaIe->cUnicastCyphers = 1;

        vos_mem_copy(&pWpaIe->UnicastOui[ 0 ], UnicastCypher, sizeof( UnicastCypher ));

        pAuthSuite = (tCsrWpaAuthIe *)( &pWpaIe->UnicastOui[ pWpaIe->cUnicastCyphers ] );

        pAuthSuite->cAuthenticationSuites = 1;
        vos_mem_copy(&pAuthSuite->AuthOui[ 0 ], AuthSuite, sizeof( AuthSuite ));

        /*
         * The WPA capabilities follows the Auth Suite (two octets)--
         * this field is optional, and we always "send" zero, so just
         * remove it.  This is consistent with our assumptions in the
         * frames compiler; c.f. bug 15234:
         * http://gold.woodsidenet.com/bugzilla/show_bug.cgi?id=15234
         *
         * Add in the fixed fields plus 1 Unicast cypher, less the IE Header
         * length
         * Add in the size of the Auth suite (count plus a single OUI)
         */
        pWpaIe->IeHeader.Length = sizeof( *pWpaIe ) - sizeof ( pWpaIe->IeHeader ) +
                                  sizeof( *pAuthSuite );

        // return the size of the IE header (total) constructed...
        cbWpaIe = pWpaIe->IeHeader.Length + sizeof( pWpaIe->IeHeader );

    } while( 0 );

    if( !pIes && pIesLocal )
    {
        //locally allocated
        vos_mem_free(pIesLocal);
    }

    return( cbWpaIe );
}


tANI_BOOLEAN csrGetWpaRsnIe( tHalHandle hHal, tANI_U8 *pIes, tANI_U32 len,
                             tANI_U8 *pWpaIe, tANI_U8 *pcbWpaIe, tANI_U8 *pRSNIe, tANI_U8 *pcbRSNIe)
{
    tDot11IEHeader *pIEHeader;
    tSirMacPropIE *pSirMacPropIE;
    tANI_U32 cbParsed;
    tANI_U32 cbIE;
    int cExpectedIEs = 0;
    int cFoundIEs = 0;
    int cbPropIETotal;

    pIEHeader = (tDot11IEHeader *)pIes;
    if(pWpaIe) cExpectedIEs++;
    if(pRSNIe) cExpectedIEs++;

    // bss description length includes all fields other than the length itself
    cbParsed  = 0;

    // Loop as long as there is data left in the IE of the Bss Description
    // and the number of Expected IEs is NOT found yet.
    while( ( (cbParsed + sizeof( *pIEHeader )) <= len ) && ( cFoundIEs < cExpectedIEs  ) )
    {
        cbIE = sizeof( *pIEHeader ) + pIEHeader->Length;

        if ( ( cbIE + cbParsed ) > len ) break;

        if ( ( pIEHeader->Length >= gCsrIELengthTable[ pIEHeader->ElementID ].min ) &&
             ( pIEHeader->Length <= gCsrIELengthTable[ pIEHeader->ElementID ].max ) )
        {
            switch( pIEHeader->ElementID )
            {
                // Parse the 221 (0xdd) Proprietary IEs here...
                // Note that the 221 IE is overloaded, containing the WPA IE, WMM/WME IE, and the
                // Airgo proprietary IE information.
                case SIR_MAC_WPA_EID:
                {
                    tANI_U32 aniOUI;
                    tANI_U8 *pOui = (tANI_U8 *)&aniOUI;

                    pOui++;
                    aniOUI = ANI_OUI;
                    aniOUI = i_ntohl( aniOUI );

                    pSirMacPropIE = ( tSirMacPropIE *)pIEHeader;
                    cbPropIETotal = pSirMacPropIE->length;

                    // Validate the ANI OUI is in the OUI field in the proprietary IE...
                    if ( ( pSirMacPropIE->length >= WNI_CFG_MANUFACTURER_OUI_LEN ) &&
                          pOui[ 0 ] == pSirMacPropIE->oui[ 0 ] &&
                          pOui[ 1 ] == pSirMacPropIE->oui[ 1 ] &&
                          pOui[ 2 ] == pSirMacPropIE->oui[ 2 ]  )
                    {
                    }
                    else
                    {
                        tCsrWpaIe     *pIe        = ( tCsrWpaIe *    )pIEHeader;

                        if(!pWpaIe || !pcbWpaIe) break;
                        // Check if this is a valid WPA IE.  Then check that the
                        // WPA OUI is in place and the version is one that we support.
                        if ( ( pIe->IeHeader.Length >= SIR_MAC_WPA_IE_MIN_LENGTH )   &&
                             ( vos_mem_compare( pIe->Oui, (void *)csrWpaOui[1],
                                                sizeof( pIe->Oui ) ) ) &&
                             ( pIe->Version <= CSR_WPA_VERSION_SUPPORTED ) )
                        {
                            vos_mem_copy(pWpaIe, pIe,
                                  pIe->IeHeader.Length + sizeof( pIe->IeHeader ));
                            *pcbWpaIe = pIe->IeHeader.Length + sizeof( pIe->IeHeader );
                            cFoundIEs++;

                            break;
                        }
                    }

                    break;
                }

                case SIR_MAC_RSN_EID:
                {
                    tCsrRSNIe *pIe;

                    if(!pcbRSNIe || !pRSNIe) break;
                    pIe = (tCsrRSNIe *)pIEHeader;

                    // Check the length of the RSN Ie to assure it is valid.  Then check that the
                    // version is one that we support.

                    if ( pIe->IeHeader.Length < SIR_MAC_RSN_IE_MIN_LENGTH ) break;
                    if ( pIe->Version > CSR_RSN_VERSION_SUPPORTED ) break;

                    cFoundIEs++;

                    // if there is enough room in the WpaIE passed in, then copy the Wpa IE into
                    // the buffer passed in.
                    if ( *pcbRSNIe < pIe->IeHeader.Length + sizeof( pIe->IeHeader ) ) break;
                    vos_mem_copy(pRSNIe, pIe,
                                 pIe->IeHeader.Length + sizeof( pIe->IeHeader ));
                    *pcbRSNIe = pIe->IeHeader.Length + sizeof( pIe->IeHeader );

                    break;
                }

                // Add support for other IE here...
                default:
                    break;
            }
        }

        cbParsed += cbIE;

        pIEHeader = (tDot11IEHeader *)( ((tANI_U8 *)pIEHeader) + cbIE );

    }

    // return a BOOL that tells if all of the IEs asked for were found...
    return( cFoundIEs == cExpectedIEs );
}


/*
 * If a WPAIE exists in the profile, just use it. Or else construct one from
 * the BSS Caller allocated memory for pWpaIe and guarantee it can contain a max
 * length WPA IE
 */
tANI_U8 csrRetrieveWpaIe( tHalHandle hHal, tCsrRoamProfile *pProfile, tSirBssDescription *pSirBssDesc,
                          tDot11fBeaconIEs *pIes, tCsrWpaIe *pWpaIe )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tANI_U8 cbWpaIe = 0;

    do
    {
        if ( !csrIsProfileWpa( pProfile ) ) break;
        if(pProfile->nWPAReqIELength && pProfile->pWPAReqIE)
        {
            if(SIR_MAC_WPA_IE_MAX_LENGTH >= pProfile->nWPAReqIELength)
            {
                cbWpaIe = (tANI_U8)pProfile->nWPAReqIELength;
                vos_mem_copy(pWpaIe, pProfile->pWPAReqIE, cbWpaIe);
            }
            else
            {
                smsLog(pMac, LOGW, "  csrRetrieveWpaIe detect invalid WPA IE length (%d) ", pProfile->nWPAReqIELength);
            }
        }
        else
        {
            cbWpaIe = csrConstructWpaIe(pMac, pProfile, pSirBssDesc, pIes, pWpaIe);
        }
    }while(0);

    return (cbWpaIe);
}


/*
 * If a RSNIE exists in the profile, just use it. Or else construct one from the
 * BSS. Caller allocated memory for pWpaIe and guarantee it can contain a max
 * length WPA IE
 */
tANI_U8 csrRetrieveRsnIe( tHalHandle hHal, tANI_U32 sessionId, tCsrRoamProfile *pProfile,
                         tSirBssDescription *pSirBssDesc, tDot11fBeaconIEs *pIes, tCsrRSNIe *pRsnIe )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tANI_U8 cbRsnIe = 0;

    do
    {
        if ( !csrIsProfileRSN( pProfile ) ) break;
#ifdef FEATURE_WLAN_LFR
        if (csrRoamIsFastRoamEnabled(pMac, sessionId))
        {
            // If "Legacy Fast Roaming" is enabled ALWAYS rebuild the RSN IE from
            // scratch. So it contains the current PMK-IDs
            cbRsnIe = csrConstructRSNIe(pMac, sessionId, pProfile, pSirBssDesc, pIes, pRsnIe);
        }
        else
#endif
        if(pProfile->nRSNReqIELength && pProfile->pRSNReqIE)
        {
            // If you have one started away, re-use it.
            if(SIR_MAC_WPA_IE_MAX_LENGTH >= pProfile->nRSNReqIELength)
            {
                cbRsnIe = (tANI_U8)pProfile->nRSNReqIELength;
                vos_mem_copy(pRsnIe, pProfile->pRSNReqIE, cbRsnIe);
            }
            else
            {
                smsLog(pMac, LOGW, "  csrRetrieveRsnIe detect invalid RSN IE length (%d) ", pProfile->nRSNReqIELength);
            }
        }
        else
        {
            cbRsnIe = csrConstructRSNIe(pMac, sessionId, pProfile, pSirBssDesc, pIes, pRsnIe);
        }
    }while(0);

    return (cbRsnIe);
}


#ifdef FEATURE_WLAN_WAPI
/*
 * If a WAPI IE exists in the profile, just use it. Or else construct one from
 * the BSS. Caller allocated memory for pWapiIe and guarantee it can contain a
 * max length WAPI IE.
 */
tANI_U8 csrRetrieveWapiIe( tHalHandle hHal, tANI_U32 sessionId,
                          tCsrRoamProfile *pProfile, tSirBssDescription *pSirBssDesc,
                          tDot11fBeaconIEs *pIes, tCsrWapiIe *pWapiIe )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tANI_U8 cbWapiIe = 0;

    do
    {
        if ( !csrIsProfileWapi( pProfile ) ) break;
        if(pProfile->nWAPIReqIELength && pProfile->pWAPIReqIE)
        {
            if(DOT11F_IE_WAPI_MAX_LEN >= pProfile->nWAPIReqIELength)
            {
                cbWapiIe = (tANI_U8)pProfile->nWAPIReqIELength;
                vos_mem_copy(pWapiIe, pProfile->pWAPIReqIE, cbWapiIe);
            }
            else
            {
                smsLog(pMac, LOGW, "  csrRetrieveWapiIe detect invalid WAPI IE length (%d) ", pProfile->nWAPIReqIELength);
            }
        }
        else
        {
            cbWapiIe = csrConstructWapiIe(pMac, sessionId, pProfile, pSirBssDesc, pIes, pWapiIe);
        }
    }while(0);

    return (cbWapiIe);
}
#endif /* FEATURE_WLAN_WAPI */

tANI_BOOLEAN csrSearchChannelListForTxPower(tHalHandle hHal, tSirBssDescription *pBssDescription, tCsrChannelSet *returnChannelGroup)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tListElem *pEntry;
    tANI_U16 i;
    tANI_U16 startingChannel;
    tANI_BOOLEAN found = FALSE;
    tCsrChannelSet *pChannelGroup;

    pEntry = csrLLPeekHead( &pMac->roam.channelList5G, LL_ACCESS_LOCK );

    while ( pEntry )
    {
        pChannelGroup = GET_BASE_ADDR( pEntry, tCsrChannelSet, channelListLink );
        startingChannel = pChannelGroup->firstChannel;
        for ( i = 0; i < pChannelGroup->numChannels; i++ )
        {
            if ( startingChannel + i * pChannelGroup->interChannelOffset == pBssDescription->channelId )
            {
                found = TRUE;
                break;
            }
        }

        if ( found )
        {
            vos_mem_copy(returnChannelGroup, pChannelGroup, sizeof(tCsrChannelSet));
            break;
        }
        else
        {
            pEntry = csrLLNext( &pMac->roam.channelList5G, pEntry, LL_ACCESS_LOCK );
        }
    }

    return( found );
}

tANI_BOOLEAN csrRatesIsDot11Rate11bSupportedRate( tANI_U8 dot11Rate )
{
    tANI_BOOLEAN fSupported = FALSE;
    tANI_U16 nonBasicRate = (tANI_U16)( BITS_OFF( dot11Rate, CSR_DOT11_BASIC_RATE_MASK ) );

    switch ( nonBasicRate )
    {
        case eCsrSuppRate_1Mbps:
        case eCsrSuppRate_2Mbps:
        case eCsrSuppRate_5_5Mbps:
        case eCsrSuppRate_11Mbps:
            fSupported = TRUE;
            break;

        default:
            break;
    }

    return( fSupported );
}

tANI_BOOLEAN csrRatesIsDot11Rate11aSupportedRate( tANI_U8 dot11Rate )
{
    tANI_BOOLEAN fSupported = FALSE;
    tANI_U16 nonBasicRate = (tANI_U16)( BITS_OFF( dot11Rate, CSR_DOT11_BASIC_RATE_MASK ) );

    switch ( nonBasicRate )
    {
        case eCsrSuppRate_6Mbps:
        case eCsrSuppRate_9Mbps:
        case eCsrSuppRate_12Mbps:
        case eCsrSuppRate_18Mbps:
        case eCsrSuppRate_24Mbps:
        case eCsrSuppRate_36Mbps:
        case eCsrSuppRate_48Mbps:
        case eCsrSuppRate_54Mbps:
            fSupported = TRUE;
            break;

        default:
            break;
    }

    return( fSupported );
}



tAniEdType csrTranslateEncryptTypeToEdType( eCsrEncryptionType EncryptType )
{
    tAniEdType edType;

    switch ( EncryptType )
    {
        default:
        case eCSR_ENCRYPT_TYPE_NONE:
            edType = eSIR_ED_NONE;
            break;

        case eCSR_ENCRYPT_TYPE_WEP40_STATICKEY:
        case eCSR_ENCRYPT_TYPE_WEP40:
            edType = eSIR_ED_WEP40;
            break;

        case eCSR_ENCRYPT_TYPE_WEP104_STATICKEY:
        case eCSR_ENCRYPT_TYPE_WEP104:
            edType = eSIR_ED_WEP104;
            break;

        case eCSR_ENCRYPT_TYPE_TKIP:
            edType = eSIR_ED_TKIP;
            break;

        case eCSR_ENCRYPT_TYPE_AES:
            edType = eSIR_ED_CCMP;
            break;
#ifdef FEATURE_WLAN_WAPI
        case eCSR_ENCRYPT_TYPE_WPI:
            edType = eSIR_ED_WPI;
            break ;
#endif
#ifdef WLAN_FEATURE_11W
        //11w BIP
        case eCSR_ENCRYPT_TYPE_AES_CMAC:
            edType = eSIR_ED_AES_128_CMAC;
            break;
#endif
    }

    return( edType );
}


//pIes can be NULL
tANI_BOOLEAN csrValidateWep( tpAniSirGlobal pMac, eCsrEncryptionType ucEncryptionType,
                             tCsrAuthList *pAuthList, tCsrEncryptionList *pMCEncryptionList,
                             eCsrAuthType *pNegotiatedAuthType, eCsrEncryptionType *pNegotiatedMCEncryption,
                             tSirBssDescription *pSirBssDesc, tDot11fBeaconIEs *pIes )
{
    tANI_U32 idx;
    tANI_BOOLEAN fMatch = FALSE;
    eCsrAuthType negotiatedAuth = eCSR_AUTH_TYPE_OPEN_SYSTEM;
    eCsrEncryptionType negotiatedMCCipher = eCSR_ENCRYPT_TYPE_UNKNOWN;

    //This function just checks whether HDD is giving correct values for Multicast cipher and Auth.

    do
    {
        //If privacy bit is not set, consider no match
        if ( !csrIsPrivacy( pSirBssDesc ) ) break;

        for( idx = 0; idx < pMCEncryptionList->numEntries; idx++ )
        {
            switch( pMCEncryptionList->encryptionType[idx] )
            {
                case eCSR_ENCRYPT_TYPE_WEP40_STATICKEY:
                case eCSR_ENCRYPT_TYPE_WEP104_STATICKEY:
                case eCSR_ENCRYPT_TYPE_WEP40:
                case eCSR_ENCRYPT_TYPE_WEP104:
                    /* Multicast list may contain WEP40/WEP104. Check whether it matches UC.
                    */
                    if( ucEncryptionType == pMCEncryptionList->encryptionType[idx] )
                    {
                        fMatch = TRUE;
                        negotiatedMCCipher = pMCEncryptionList->encryptionType[idx];
                    }
                    break;
                default:
                    fMatch = FALSE;
                    break;
            }
            if(fMatch) break;
        }

        if(!fMatch) break;

        for( idx = 0; idx < pAuthList->numEntries; idx++ )
        {
            switch( pAuthList->authType[idx] )
            {
                case eCSR_AUTH_TYPE_OPEN_SYSTEM:
                case eCSR_AUTH_TYPE_SHARED_KEY:
                case eCSR_AUTH_TYPE_AUTOSWITCH:
                    fMatch = TRUE;
                    negotiatedAuth = pAuthList->authType[idx];
                    break;
                default:
                    fMatch = FALSE;
            }
            if (fMatch) break;
        }

        if(!fMatch) break;
        //In case of WPA / WPA2, check whether it supports WEP as well
        if(pIes)
        {
            //Prepare the encryption type for WPA/WPA2 functions
            if( eCSR_ENCRYPT_TYPE_WEP40_STATICKEY == ucEncryptionType )
            {
                ucEncryptionType = eCSR_ENCRYPT_TYPE_WEP40;
            }
            else if( eCSR_ENCRYPT_TYPE_WEP104 == ucEncryptionType )
            {
                ucEncryptionType = eCSR_ENCRYPT_TYPE_WEP104;
            }
            //else we can use the encryption type directly
            if ( pIes->WPA.present )
            {
                fMatch = vos_mem_compare(pIes->WPA.multicast_cipher,
                                         csrWpaOui[csrGetOUIIndexFromCipher( ucEncryptionType )],
                                         CSR_WPA_OUI_SIZE);
                if( fMatch ) break;
            }
            if ( pIes->RSN.present )
            {
                fMatch = vos_mem_compare(pIes->RSN.gp_cipher_suite,
                                         csrRSNOui[csrGetOUIIndexFromCipher( ucEncryptionType )],
                                         CSR_RSN_OUI_SIZE);
            }
        }

    }while(0);

    if( fMatch )
    {
        if( pNegotiatedAuthType )
            *pNegotiatedAuthType = negotiatedAuth;

        if( pNegotiatedMCEncryption )
            *pNegotiatedMCEncryption = negotiatedMCCipher;
    }


    return fMatch;
}


//pIes shall contain IEs from pSirBssDesc. It shall be returned from function csrGetParsedBssDescriptionIEs
tANI_BOOLEAN csrIsSecurityMatch( tHalHandle hHal, tCsrAuthList *authType,
                                 tCsrEncryptionList *pUCEncryptionType,
                                 tCsrEncryptionList *pMCEncryptionType,
                                 tANI_BOOLEAN *pMFPEnabled,
                                 tANI_U8 *pMFPRequired, tANI_U8 *pMFPCapable,
                                 tSirBssDescription *pSirBssDesc,
                                 tDot11fBeaconIEs *pIes,
                                 eCsrAuthType *negotiatedAuthtype,
                                 eCsrEncryptionType *negotiatedUCCipher,
                                 eCsrEncryptionType *negotiatedMCCipher )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tANI_BOOLEAN fMatch = FALSE;
    tANI_U8 i,idx;
    eCsrEncryptionType mcCipher = eCSR_ENCRYPT_TYPE_UNKNOWN, ucCipher = eCSR_ENCRYPT_TYPE_UNKNOWN;
    eCsrAuthType negAuthType = eCSR_AUTH_TYPE_UNKNOWN;

    for( i = 0 ; ((i < pUCEncryptionType->numEntries) && (!fMatch)) ; i++ )
    {
        ucCipher = pUCEncryptionType->encryptionType[i];
        // If the Bss description shows the Privacy bit is on, then we must have some sort of encryption configured
        // for the profile to work.  Don't attempt to join networks with Privacy bit set when profiles say NONE for
        // encryption type.
        switch ( ucCipher )
        {
            case eCSR_ENCRYPT_TYPE_NONE:
                {
                    // for NO encryption, if the Bss description has the Privacy bit turned on, then encryption is
                    // required so we have to reject this Bss.
                    if ( csrIsPrivacy( pSirBssDesc ) )
                    {
                        fMatch = FALSE;
                    }
                    else
                    {
                        fMatch = TRUE;
                    }

                    if ( fMatch )
                    {
                        fMatch = FALSE;
                        //Check Multicast cipher requested and Auth type requested.
                        for( idx = 0 ; idx < pMCEncryptionType->numEntries ; idx++ )
                        {
                            if( eCSR_ENCRYPT_TYPE_NONE == pMCEncryptionType->encryptionType[idx] )
                            {
                                fMatch = TRUE; //Multicast can only be none.
                                mcCipher = pMCEncryptionType->encryptionType[idx];
                                break;
                            }
                        }
                        if (!fMatch) break;

                        fMatch = FALSE;
                        //Check Auth list. It should contain AuthOpen.
                        for( idx = 0 ; idx < authType->numEntries ; idx++ )
                        {
                            if(( eCSR_AUTH_TYPE_OPEN_SYSTEM == authType->authType[idx] ) ||
                               ( eCSR_AUTH_TYPE_AUTOSWITCH == authType->authType[idx] ))
                            {
                               fMatch = TRUE;
                               negAuthType = eCSR_AUTH_TYPE_OPEN_SYSTEM;
                               break;
                            }
                        }
                        if (!fMatch) break;

                    }
                    break;
                }

            case eCSR_ENCRYPT_TYPE_WEP40_STATICKEY:
            case eCSR_ENCRYPT_TYPE_WEP104_STATICKEY:
                // !! might want to check for WEP keys set in the Profile.... ?
                // !! don't need to have the privacy bit in the Bss description.  Many AP policies make legacy
                // encryption 'optional' so we don't know if we can associate or not.  The AP will reject if
                // encryption is not allowed without the Privacy bit turned on.
                fMatch = csrValidateWep( pMac, ucCipher, authType, pMCEncryptionType, &negAuthType, &mcCipher, pSirBssDesc, pIes);

                break;

                // these are all of the WPA encryption types...
            case eCSR_ENCRYPT_TYPE_WEP40:
            case eCSR_ENCRYPT_TYPE_WEP104:
                fMatch = csrValidateWep( pMac, ucCipher, authType, pMCEncryptionType, &negAuthType, &mcCipher, pSirBssDesc, pIes);
                break;

            case eCSR_ENCRYPT_TYPE_TKIP:
            case eCSR_ENCRYPT_TYPE_AES:
                {
                    if(pIes)
                    {
                        // First check if there is a RSN match
                        fMatch = csrIsRSNMatch( pMac, authType, ucCipher,
                                                pMCEncryptionType, pMFPEnabled,
                                                pMFPRequired, pMFPCapable,
                                                pIes, &negAuthType, &mcCipher );
                        if( !fMatch )
                        {
                            // If not RSN, then check if there is a WPA match
                            fMatch = csrIsWpaEncryptionMatch( pMac, authType, ucCipher, pMCEncryptionType, pIes,
                                                              &negAuthType, &mcCipher );
                        }
                    }
                    else
                    {
                        fMatch = FALSE;
                    }
                    break;
                }
#ifdef FEATURE_WLAN_WAPI
           case eCSR_ENCRYPT_TYPE_WPI://WAPI
               {
                   if(pIes)
                   {
                       fMatch = csrIsWapiMatch( hHal, authType, ucCipher, pMCEncryptionType, pIes, &negAuthType, &mcCipher );
                   }
                   else
                   {
                       fMatch = FALSE;
                   }
                   break;
               }
#endif /* FEATURE_WLAN_WAPI */
            case eCSR_ENCRYPT_TYPE_ANY:
            default:
            {
                tANI_BOOLEAN fMatchAny = eANI_BOOLEAN_FALSE;

                fMatch = eANI_BOOLEAN_TRUE;
                //It is allowed to match anything. Try the more secured ones first.
                if(pIes)
                {
                    //Check AES first
                    ucCipher = eCSR_ENCRYPT_TYPE_AES;
                    fMatchAny = csrIsRSNMatch( hHal, authType, ucCipher,
                                               pMCEncryptionType, pMFPEnabled,
                                               pMFPRequired, pMFPCapable, pIes,
                                               &negAuthType, &mcCipher );
                    if(!fMatchAny)
                    {
                        //Check TKIP
                        ucCipher = eCSR_ENCRYPT_TYPE_TKIP;
                        fMatchAny = csrIsRSNMatch( hHal, authType, ucCipher,
                                                   pMCEncryptionType,
                                                   pMFPEnabled, pMFPRequired,
                                                   pMFPCapable, pIes,
                                                   &negAuthType, &mcCipher );
                    }
#ifdef FEATURE_WLAN_WAPI
                    if(!fMatchAny)
                    {
                        //Check WAPI
                        ucCipher = eCSR_ENCRYPT_TYPE_WPI;
                        fMatchAny = csrIsWapiMatch( hHal, authType, ucCipher, pMCEncryptionType, pIes, &negAuthType, &mcCipher );
                    }
#endif /* FEATURE_WLAN_WAPI */
                }
                if(!fMatchAny)
                {
                    ucCipher = eCSR_ENCRYPT_TYPE_WEP104;
                    if(!csrValidateWep( pMac, ucCipher, authType, pMCEncryptionType, &negAuthType, &mcCipher, pSirBssDesc, pIes))
                    {
                        ucCipher = eCSR_ENCRYPT_TYPE_WEP40;
                        if(!csrValidateWep( pMac, ucCipher, authType, pMCEncryptionType, &negAuthType, &mcCipher, pSirBssDesc, pIes))
                        {
                            ucCipher = eCSR_ENCRYPT_TYPE_WEP104_STATICKEY;
                            if(!csrValidateWep( pMac, ucCipher, authType, pMCEncryptionType, &negAuthType, &mcCipher, pSirBssDesc, pIes))
                            {
                                ucCipher = eCSR_ENCRYPT_TYPE_WEP40_STATICKEY;
                                if(!csrValidateWep( pMac, ucCipher, authType, pMCEncryptionType, &negAuthType, &mcCipher, pSirBssDesc, pIes))
                                {
                                    //It must be open and no encryption
                                    if ( csrIsPrivacy( pSirBssDesc ) )
                                    {
                                        //This is not right
                                        fMatch = eANI_BOOLEAN_FALSE;
                                    }
                                    else
                                    {
                                        negAuthType = eCSR_AUTH_TYPE_OPEN_SYSTEM;
                                        mcCipher = eCSR_ENCRYPT_TYPE_NONE;
                                        ucCipher = eCSR_ENCRYPT_TYPE_NONE;
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            }
        }

    }

    if( fMatch )
    {
        if( negotiatedUCCipher )
            *negotiatedUCCipher = ucCipher;

        if( negotiatedMCCipher )
            *negotiatedMCCipher = mcCipher;

        if( negotiatedAuthtype )
            *negotiatedAuthtype = negAuthType;
    }

    return( fMatch );
}


tANI_BOOLEAN csrIsSsidMatch( tpAniSirGlobal pMac, tANI_U8 *ssid1, tANI_U8 ssid1Len, tANI_U8 *bssSsid,
                            tANI_U8 bssSsidLen, tANI_BOOLEAN fSsidRequired )
{
    tANI_BOOLEAN fMatch = FALSE;

    do {
        /*
         * Check for the specification of the Broadcast SSID at the beginning
         * of the list. If specified, then all SSIDs are matches
         * (broadcast SSID means accept all SSIDs).
         */
        if ( ssid1Len == 0 )
        {
            fMatch = TRUE;
            break;
        }

        /*
         * There are a few special cases. If the Bss description has a
         * Broadcast SSID, then our Profile must have a single SSID without
         * Wild cards so we can program the SSID.
         * SSID could be suppressed in beacons. In that case SSID IE has valid
         * length but the SSID value is all NULL characters.
         * That condition is treated same as NULL SSID.
         */
        if ( csrIsNULLSSID( bssSsid, bssSsidLen ) )
        {
            if ( eANI_BOOLEAN_FALSE == fSsidRequired )
            {
                fMatch = TRUE;
                break;
            }
        }

        if(ssid1Len != bssSsidLen) break;
        if (vos_mem_compare(bssSsid, ssid1, bssSsidLen))
        {
            fMatch = TRUE;
            break;
        }

    } while( 0 );

    return( fMatch );
}


//Null ssid means match
tANI_BOOLEAN csrIsSsidInList( tHalHandle hHal, tSirMacSSid *pSsid, tCsrSSIDs *pSsidList )
{
    tANI_BOOLEAN fMatch = FALSE;
    tANI_U32 i;

    if ( pSsidList && pSsid )
    {
        for(i = 0; i < pSsidList->numOfSSIDs; i++)
        {
            if(csrIsNULLSSID(pSsidList->SSIDList[i].SSID.ssId, pSsidList->SSIDList[i].SSID.length) ||
              ((pSsidList->SSIDList[i].SSID.length == pSsid->length) &&
               vos_mem_compare(pSsid->ssId, pSsidList->SSIDList[i].SSID.ssId, pSsid->length)))
            {
                fMatch = TRUE;
                break;
            }
        }
    }

    return (fMatch);
}

//like to use sirCompareMacAddr
tANI_BOOLEAN csrIsMacAddressZero( tpAniSirGlobal pMac, tCsrBssid *pMacAddr )
{
    tANI_U8 bssid[VOS_MAC_ADDR_SIZE] = {0, 0, 0, 0, 0, 0};

    return (vos_mem_compare(bssid, pMacAddr, VOS_MAC_ADDR_SIZE));
}

//like to use sirCompareMacAddr
tANI_BOOLEAN csrIsMacAddressBroadcast( tpAniSirGlobal pMac, tCsrBssid *pMacAddr )
{
    tANI_U8 bssid[VOS_MAC_ADDR_SIZE] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    return(vos_mem_compare(bssid, pMacAddr, VOS_MAC_ADDR_SIZE));
}


//like to use sirCompareMacAddr
tANI_BOOLEAN csrIsMacAddressEqual( tpAniSirGlobal pMac, tCsrBssid *pMacAddr1, tCsrBssid *pMacAddr2 )
{
    return(vos_mem_compare(pMacAddr1, pMacAddr2, sizeof(tCsrBssid)));
}


tANI_BOOLEAN csrIsBssidMatch( tHalHandle hHal, tCsrBssid *pProfBssid, tCsrBssid *BssBssid )
{
    tANI_BOOLEAN fMatch = FALSE;
    tCsrBssid ProfileBssid;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    // for efficiency of the MAC_ADDRESS functions, move the
    // Bssid's into MAC_ADDRESS structs.
    vos_mem_copy(&ProfileBssid, pProfBssid, sizeof(tCsrBssid));

    do {

        // Give the profile the benefit of the doubt... accept either all 0 or
        // the real broadcast Bssid (all 0xff) as broadcast Bssids (meaning to
        // match any Bssids).
        if ( csrIsMacAddressZero( pMac, &ProfileBssid ) ||
             csrIsMacAddressBroadcast( pMac, &ProfileBssid ) )
        {
             fMatch = TRUE;
             break;
        }

        if ( csrIsMacAddressEqual( pMac, BssBssid, &ProfileBssid ) )
        {
            fMatch = TRUE;
            break;
        }

    } while( 0 );

    return( fMatch );
}


tANI_BOOLEAN csrIsBSSTypeMatch(eCsrRoamBssType bssType1, eCsrRoamBssType bssType2)
{
    if((eCSR_BSS_TYPE_ANY != bssType1 && eCSR_BSS_TYPE_ANY != bssType2) && (bssType1 != bssType2))
        return eANI_BOOLEAN_FALSE;
    else
        return eANI_BOOLEAN_TRUE;
}


tANI_BOOLEAN csrIsBssTypeIBSS(eCsrRoamBssType bssType)
{
    return((tANI_BOOLEAN)(eCSR_BSS_TYPE_START_IBSS == bssType || eCSR_BSS_TYPE_IBSS == bssType));
}

tANI_BOOLEAN csrIsBssTypeWDS(eCsrRoamBssType bssType)
{
    return((tANI_BOOLEAN)(eCSR_BSS_TYPE_WDS_STA == bssType || eCSR_BSS_TYPE_WDS_AP == bssType));
}

tANI_BOOLEAN csrIsBSSTypeCapsMatch( eCsrRoamBssType bssType, tSirBssDescription *pSirBssDesc )
{
    tANI_BOOLEAN fMatch = TRUE;

    do
    {
        switch( bssType )
        {
            case eCSR_BSS_TYPE_ANY:
                break;

            case eCSR_BSS_TYPE_INFRASTRUCTURE:
            case eCSR_BSS_TYPE_WDS_STA:
                if( !csrIsInfraBssDesc( pSirBssDesc ) )
                    fMatch = FALSE;

                break;

            case eCSR_BSS_TYPE_IBSS:
            case eCSR_BSS_TYPE_START_IBSS:
                if( !csrIsIbssBssDesc( pSirBssDesc ) )
                    fMatch = FALSE;

                break;

            case eCSR_BSS_TYPE_WDS_AP: //For WDS AP, no need to match anything
            default:
                fMatch = FALSE;
                break;
        }
    }
    while( 0 );


    return( fMatch );
}

static tANI_BOOLEAN csrIsCapabilitiesMatch( tpAniSirGlobal pMac, eCsrRoamBssType bssType, tSirBssDescription *pSirBssDesc )
{
  return( csrIsBSSTypeCapsMatch( bssType, pSirBssDesc ) );
}



static tANI_BOOLEAN csrIsSpecificChannelMatch( tpAniSirGlobal pMac, tSirBssDescription *pSirBssDesc, tANI_U8 Channel )
{
    tANI_BOOLEAN fMatch = TRUE;

    do
    {
        // if the channel is ANY, then always match...
        if ( eCSR_OPERATING_CHANNEL_ANY == Channel ) break;
        if ( Channel == pSirBssDesc->channelId ) break;

        // didn't match anything.. so return NO match
        fMatch = FALSE;

    } while( 0 );

    return( fMatch );
}


tANI_BOOLEAN csrIsChannelBandMatch( tpAniSirGlobal pMac, tANI_U8 channelId, tSirBssDescription *pSirBssDesc )
{
    tANI_BOOLEAN fMatch = TRUE;

    do
    {
        // if the profile says Any channel AND the global settings says ANY channel, then we
        // always match...
        if ( eCSR_OPERATING_CHANNEL_ANY == channelId ) break;

        if ( eCSR_OPERATING_CHANNEL_ANY != channelId )
        {
            fMatch = csrIsSpecificChannelMatch( pMac, pSirBssDesc, channelId );
        }

    } while( 0 );

    return( fMatch );
}


/**
 * \brief Enquire as to whether a given rate is supported by the
 * adapter as currently configured
 *
 *
 * \param nRate A rate in units of 500kbps
 *
 * \return TRUE if  the adapter is currently capable  of supporting this
 * rate, FALSE else
 *
 *
 * The rate encoding  is just as in 802.11  Information Elements, except
 * that the high bit is \em  not interpreted as indicating a Basic Rate,
 * and proprietary rates are allowed, too.
 *
 * Note  that if the  adapter's dot11Mode  is g,  we don't  restrict the
 * rates.  According to hwReadEepromParameters, this will happen when:
 *
 *   ... the  card is  configured for ALL  bands through  the property
 *   page.  If this occurs, and the card is not an ABG card ,then this
 *   code  is  setting the  dot11Mode  to  assume  the mode  that  the
 *   hardware can support.   For example, if the card  is an 11BG card
 *   and we  are configured to support  ALL bands, then  we change the
 *   dot11Mode  to 11g  because  ALL in  this  case is  only what  the
 *   hardware can support.
 *
 *
 */

static tANI_BOOLEAN csrIsAggregateRateSupported( tpAniSirGlobal pMac, tANI_U16 rate )
{
    tANI_BOOLEAN fSupported = eANI_BOOLEAN_FALSE;
    tANI_U16 idx, newRate;

    //In case basic rate flag is set
    newRate = BITS_OFF(rate, CSR_DOT11_BASIC_RATE_MASK);
    if ( eCSR_CFG_DOT11_MODE_11A == pMac->roam.configParam.uCfgDot11Mode )
    {
        switch ( newRate )
        {
        case eCsrSuppRate_6Mbps:
        case eCsrSuppRate_9Mbps:
        case eCsrSuppRate_12Mbps:
        case eCsrSuppRate_18Mbps:
        case eCsrSuppRate_24Mbps:
        case eCsrSuppRate_36Mbps:
        case eCsrSuppRate_48Mbps:
        case eCsrSuppRate_54Mbps:
            fSupported = TRUE;
            break;
        default:
            fSupported = FALSE;
            break;
        }

    }
    else if( eCSR_CFG_DOT11_MODE_11B == pMac->roam.configParam.uCfgDot11Mode )
    {
        switch ( newRate )
        {
        case eCsrSuppRate_1Mbps:
        case eCsrSuppRate_2Mbps:
        case eCsrSuppRate_5_5Mbps:
        case eCsrSuppRate_11Mbps:
            fSupported = TRUE;
            break;
        default:
            fSupported = FALSE;
            break;
        }
    }
    else if ( !pMac->roam.configParam.ProprietaryRatesEnabled )
    {

        switch ( newRate )
        {
        case eCsrSuppRate_1Mbps:
        case eCsrSuppRate_2Mbps:
        case eCsrSuppRate_5_5Mbps:
        case eCsrSuppRate_6Mbps:
        case eCsrSuppRate_9Mbps:
        case eCsrSuppRate_11Mbps:
        case eCsrSuppRate_12Mbps:
        case eCsrSuppRate_18Mbps:
        case eCsrSuppRate_24Mbps:
        case eCsrSuppRate_36Mbps:
        case eCsrSuppRate_48Mbps:
        case eCsrSuppRate_54Mbps:
            fSupported = TRUE;
            break;
        default:
            fSupported = FALSE;
            break;
        }

    }
    else {

        if ( eCsrSuppRate_1Mbps   == newRate ||
             eCsrSuppRate_2Mbps   == newRate ||
             eCsrSuppRate_5_5Mbps == newRate ||
             eCsrSuppRate_11Mbps  == newRate )
        {
            fSupported = TRUE;
        }
        else {
            idx = 0x1;

            switch ( newRate )
            {
            case eCsrSuppRate_6Mbps:
                fSupported = gPhyRatesSuppt[0][idx];
                break;
            case eCsrSuppRate_9Mbps:
                fSupported = gPhyRatesSuppt[1][idx];
                break;
            case eCsrSuppRate_12Mbps:
                fSupported = gPhyRatesSuppt[2][idx];
                break;
            case eCsrSuppRate_18Mbps:
                fSupported = gPhyRatesSuppt[3][idx];
                break;
            case eCsrSuppRate_20Mbps:
                fSupported = gPhyRatesSuppt[4][idx];
                break;
            case eCsrSuppRate_24Mbps:
                fSupported = gPhyRatesSuppt[5][idx];
                break;
            case eCsrSuppRate_36Mbps:
                fSupported = gPhyRatesSuppt[6][idx];
                break;
            case eCsrSuppRate_40Mbps:
                fSupported = gPhyRatesSuppt[7][idx];
                break;
            case eCsrSuppRate_42Mbps:
                fSupported = gPhyRatesSuppt[8][idx];
                break;
            case eCsrSuppRate_48Mbps:
                fSupported = gPhyRatesSuppt[9][idx];
                break;
            case eCsrSuppRate_54Mbps:
                fSupported = gPhyRatesSuppt[10][idx];
                break;
            case eCsrSuppRate_72Mbps:
                fSupported = gPhyRatesSuppt[11][idx];
                break;
            case eCsrSuppRate_80Mbps:
                fSupported = gPhyRatesSuppt[12][idx];
                break;
            case eCsrSuppRate_84Mbps:
                fSupported = gPhyRatesSuppt[13][idx];
                break;
            case eCsrSuppRate_96Mbps:
                fSupported = gPhyRatesSuppt[14][idx];
                break;
            case eCsrSuppRate_108Mbps:
                fSupported = gPhyRatesSuppt[15][idx];
                break;
            case eCsrSuppRate_120Mbps:
                fSupported = gPhyRatesSuppt[16][idx];
                break;
            case eCsrSuppRate_126Mbps:
                fSupported = gPhyRatesSuppt[17][idx];
                break;
            case eCsrSuppRate_144Mbps:
                fSupported = gPhyRatesSuppt[18][idx];
                break;
            case eCsrSuppRate_160Mbps:
                fSupported = gPhyRatesSuppt[19][idx];
                break;
            case eCsrSuppRate_168Mbps:
                fSupported = gPhyRatesSuppt[20][idx];
                break;
            case eCsrSuppRate_192Mbps:
                fSupported = gPhyRatesSuppt[21][idx];
                break;
            case eCsrSuppRate_216Mbps:
                fSupported = gPhyRatesSuppt[22][idx];
                break;
            case eCsrSuppRate_240Mbps:
                fSupported = gPhyRatesSuppt[23][idx];
                break;
            default:
                fSupported = FALSE;
                break;
            }
        }
    }

    return fSupported;
}



static tANI_BOOLEAN csrIsRateSetMatch( tpAniSirGlobal pMac,
                                     tDot11fIESuppRates *pBssSuppRates,
                                     tDot11fIEExtSuppRates *pBssExtSuppRates )
{
    tANI_BOOLEAN fMatch = TRUE;
    tANI_U32 i;


    // Validate that all of the Basic rates advertised in the Bss description are supported.
    if ( pBssSuppRates )
    {
        for( i = 0; i < pBssSuppRates->num_rates; i++ )
        {
            if ( CSR_IS_BASIC_RATE( pBssSuppRates->rates[ i ] ) )
            {
                if ( !csrIsAggregateRateSupported( pMac, pBssSuppRates->rates[ i ] ) )
                {
                    fMatch = FALSE;
                    break;
                }
            }
        }
    }

    if ( fMatch && pBssExtSuppRates )
    {
        for( i = 0; i < pBssExtSuppRates->num_rates; i++ )
        {
            if ( CSR_IS_BASIC_RATE( pBssExtSuppRates->rates[ i ] ) )
            {
                if ( !csrIsAggregateRateSupported( pMac, pBssExtSuppRates->rates[ i ] ) )
                {
                    fMatch = FALSE;
                    break;
                }
            }
        }
    }

    return( fMatch );

}

#ifdef WLAN_FEATURE_FILS_SK
/*
 * csr_is_fils_realm_match: API to check whether realm in scan filter is
 * matching with realm in bss info
 * @bss_descr: bss description
 * @filter: scan filter
 *
 * Return: true if success else false
 */
static bool csr_is_fils_realm_match(tSirBssDescription *bss_descr,
                        tCsrScanResultFilter *filter)
{
    int i;
    bool is_match = true;

    if (filter->realm_check) {
        is_match = false;
        for (i = 0; i < bss_descr->fils_info_element.realm_cnt; i++) {
            if (!adf_os_mem_cmp(filter->fils_realm,
                    bss_descr->fils_info_element.realm[i],
                    SIR_REALM_LEN)) {
                    return true;
            }
        }
    }
    return is_match;
}
#else
static bool csr_is_fils_realm_match(tSirBssDescription *bss_descr,
                                    tCsrScanResultFilter *filter)
{
    return true;
}
#endif

//ppIes can be NULL. If caller want to get the *ppIes allocated by this function, pass in *ppIes = NULL
tANI_BOOLEAN csrMatchBSS( tHalHandle hHal, tSirBssDescription *pBssDesc, tCsrScanResultFilter *pFilter,
                          eCsrAuthType *pNegAuth, eCsrEncryptionType *pNegUc, eCsrEncryptionType *pNegMc,
                          tDot11fBeaconIEs **ppIes)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tANI_BOOLEAN fRC = eANI_BOOLEAN_FALSE, fCheck, blacklist_check;
    tANI_U32 i;
    tDot11fBeaconIEs *pIes = NULL;
    tANI_U8 *pb;
    tCsrBssid *blacklist_bssid = NULL;
    struct roam_ext_params *roam_params;

    roam_params = &pMac->roam.configParam.roam_params;
    do {
        if( ( NULL == ppIes ) || ( *ppIes ) == NULL )
        {
            //If no IEs passed in, get our own.
            if(!HAL_STATUS_SUCCESS(csrGetParsedBssDescriptionIEs(pMac, pBssDesc, &pIes)))
            {
                break;
            }
        }
        else
        {
            //Save the one pass in for local use
            pIes = *ppIes;
        }

        //Check if caller wants P2P
        fCheck = (!pFilter->p2pResult || pIes->P2PBeaconProbeRes.present);
        if(!fCheck) break;

        /* Check for Blacklist BSSID's and avoid connections */
        blacklist_check = false;
        blacklist_bssid = (tCsrBssid *)&roam_params->bssid_avoid_list;
        for (i = 0; i < roam_params->num_bssid_avoid_list; i++) {
          if (csrIsMacAddressEqual(pMac, blacklist_bssid,
               (tCsrBssid *)pBssDesc->bssId)) {
                 blacklist_check = true;
                 break;
          }
          blacklist_bssid++;
        }
        if(blacklist_check) {
            VOS_TRACE(VOS_MODULE_ID_SME, VOS_TRACE_LEVEL_ERROR,
              "Do not Attempt connection to blacklist bssid");
            break;
        }

	if(pIes->SSID.present)
        {
            for(i = 0; i < pFilter->SSIDs.numOfSSIDs; i++)
            {
                fCheck = csrIsSsidMatch( pMac, pFilter->SSIDs.SSIDList[i].SSID.ssId, pFilter->SSIDs.SSIDList[i].SSID.length,
                                        pIes->SSID.ssid,
                                        pIes->SSID.num_ssid, eANI_BOOLEAN_TRUE );
                if ( fCheck ) break;
            }
            if(!fCheck) break;
        }

#ifndef NO_SILEX_CHANGE
        /* If there is no SSID IE, treat it as "not matched". */
        else
        {
            break;
        }
#endif /* NO_SILEX_CHANGE */

        fCheck = eANI_BOOLEAN_TRUE;
        for(i = 0; i < pFilter->BSSIDs.numOfBSSIDs; i++)
        {
            fCheck = csrIsBssidMatch( pMac, (tCsrBssid *)&pFilter->BSSIDs.bssid[i], (tCsrBssid *)pBssDesc->bssId );
            if ( fCheck ) break;

            if (pFilter->p2pResult && pIes->P2PBeaconProbeRes.present)
            {
               fCheck = csrIsBssidMatch( pMac, (tCsrBssid *)&pFilter->BSSIDs.bssid[i],
                              (tCsrBssid *)pIes->P2PBeaconProbeRes.P2PDeviceInfo.P2PDeviceAddress );

               if ( fCheck ) break;
            }
        }
        if(!fCheck) break;
        fCheck = eANI_BOOLEAN_TRUE;
        for(i = 0; i < pFilter->ChannelInfo.numOfChannels; i++)
        {
            fCheck = csrIsChannelBandMatch( pMac, pFilter->ChannelInfo.ChannelList[i], pBssDesc );
            if ( fCheck ) break;
        }
        if(!fCheck)
            break;
#if defined WLAN_FEATURE_VOWIFI
        /* If this is for measurement filtering */
        if( pFilter->fMeasurement )
        {
           fRC = eANI_BOOLEAN_TRUE;
           break;
        }
#endif
        if ( !csrIsPhyModeMatch( pMac, pFilter->phyMode, pBssDesc, NULL, NULL, pIes ) ) break;
        if ( (!pFilter->bWPSAssociation) && (!pFilter->bOSENAssociation) &&
#ifdef WLAN_FEATURE_11W
             !csrIsSecurityMatch( pMac, &pFilter->authType,
                                  &pFilter->EncryptionType,
                                  &pFilter->mcEncryptionType,
                                  &pFilter->MFPEnabled,
                                  &pFilter->MFPRequired,
                                  &pFilter->MFPCapable,
                                  pBssDesc, pIes, pNegAuth,
                                  pNegUc, pNegMc )
#else
             !csrIsSecurityMatch( pMac, &pFilter->authType,
                                  &pFilter->EncryptionType,
                                  &pFilter->mcEncryptionType,
                                  NULL, NULL, NULL,
                                  pBssDesc, pIes, pNegAuth,
                                  pNegUc, pNegMc )
#endif
                                                   ) break;
        if ( !csrIsCapabilitiesMatch( pMac, pFilter->BSSType, pBssDesc ) ) break;
        if ( !csrIsRateSetMatch( pMac, &pIes->SuppRates, &pIes->ExtSuppRates ) ) break;
        //Tush-QoS: validate first if asked for APSD or WMM association
        if ( (eCsrRoamWmmQbssOnly == pMac->roam.configParam.WMMSupportMode) &&
             !CSR_IS_QOS_BSS(pIes) )
             break;
        //Check country. check even when pb is NULL because we may want to make sure
        //AP has a country code in it if fEnforceCountryCodeMatch is set.
        pb = ( pFilter->countryCode[0] ) ? ( pFilter->countryCode) : NULL;

        fCheck = csrMatchCountryCode( pMac, pb, pIes );
        if(!fCheck)
            break;

#ifdef WLAN_FEATURE_VOWIFI_11R
        if (pFilter->MDID.mdiePresent &&
                 csrRoamIs11rAssoc(pMac, pMac->roam.roamSession->sessionId)) {
            if (pBssDesc->mdiePresent)
            {
                if (pFilter->MDID.mobilityDomain != (pBssDesc->mdie[1] << 8 | pBssDesc->mdie[0]))
                    break;
            }
            else
                break;
        }
#endif
        fRC = eANI_BOOLEAN_TRUE;
        if (fRC)
            fRC = csr_is_fils_realm_match(pBssDesc, pFilter);

    } while( 0 );
    if( ppIes )
    {
        *ppIes = pIes;
    }
    else if( pIes )
    {
        vos_mem_free(pIes);
    }

    return( fRC );
}

tANI_BOOLEAN csrMatchConnectedBSSSecurity( tpAniSirGlobal pMac, tCsrRoamConnectedProfile *pProfile,
                                           tSirBssDescription *pBssDesc, tDot11fBeaconIEs *pIes)
{
    tCsrEncryptionList ucEncryptionList, mcEncryptionList;
    tCsrAuthList authList;

    ucEncryptionList.numEntries = 1;
    ucEncryptionList.encryptionType[0] = pProfile->EncryptionType;

    mcEncryptionList.numEntries = 1;
    mcEncryptionList.encryptionType[0] = pProfile->mcEncryptionType;

    authList.numEntries = 1;
    authList.authType[0] = pProfile->AuthType;

    return( csrIsSecurityMatch( pMac, &authList, &ucEncryptionList,
                                &mcEncryptionList, NULL, NULL, NULL,
                                pBssDesc, pIes, NULL, NULL, NULL ));

}


tANI_BOOLEAN csrMatchBSSToConnectProfile( tHalHandle hHal, tCsrRoamConnectedProfile *pProfile,
                                          tSirBssDescription *pBssDesc, tDot11fBeaconIEs *pIes )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tANI_BOOLEAN fRC = eANI_BOOLEAN_FALSE, fCheck;
    tDot11fBeaconIEs *pIesLocal = pIes;

    do {
        if( !pIes )
        {
            if(!HAL_STATUS_SUCCESS(csrGetParsedBssDescriptionIEs(pMac, pBssDesc, &pIesLocal)))
            {
                break;
            }
        }
        fCheck = eANI_BOOLEAN_TRUE;
        if(pIesLocal->SSID.present)
        {
            tANI_BOOLEAN fCheckSsid = eANI_BOOLEAN_FALSE;
            if(pProfile->SSID.length)
            {
                fCheckSsid = eANI_BOOLEAN_TRUE;
            }
            fCheck = csrIsSsidMatch( pMac, pProfile->SSID.ssId, pProfile->SSID.length,
                                        pIesLocal->SSID.ssid, pIesLocal->SSID.num_ssid, fCheckSsid );
            if(!fCheck) break;
        }
        if ( !csrMatchConnectedBSSSecurity( pMac, pProfile, pBssDesc, pIesLocal) ) break;
        if ( !csrIsCapabilitiesMatch( pMac, pProfile->BSSType, pBssDesc ) ) break;
        if ( !csrIsRateSetMatch( pMac, &pIesLocal->SuppRates, &pIesLocal->ExtSuppRates ) ) break;
        fCheck = csrIsChannelBandMatch( pMac, pProfile->operationChannel, pBssDesc );
        if(!fCheck)
            break;

        fRC = eANI_BOOLEAN_TRUE;

    } while( 0 );

    if( !pIes && pIesLocal )
    {
        //locally allocated
        vos_mem_free(pIesLocal);
    }

    return( fRC );
}

void csrAddRateBitmap(tANI_U8 rate, tANI_U16 *pRateBitmap)
{
    tANI_U16 rateBitmap;
    tANI_U16 n = BITS_OFF( rate, CSR_DOT11_BASIC_RATE_MASK );
    rateBitmap = *pRateBitmap;
    switch(n)
    {
       case SIR_MAC_RATE_1:
            rateBitmap |= SIR_MAC_RATE_1_BITMAP;
            break;
        case SIR_MAC_RATE_2:
            rateBitmap |= SIR_MAC_RATE_2_BITMAP;
            break;
        case SIR_MAC_RATE_5_5:
            rateBitmap |= SIR_MAC_RATE_5_5_BITMAP;
            break;
        case SIR_MAC_RATE_11:
            rateBitmap |= SIR_MAC_RATE_11_BITMAP;
            break;
        case SIR_MAC_RATE_6:
            rateBitmap |= SIR_MAC_RATE_6_BITMAP;
            break;
        case SIR_MAC_RATE_9:
            rateBitmap |= SIR_MAC_RATE_9_BITMAP;
            break;
        case SIR_MAC_RATE_12:
            rateBitmap |= SIR_MAC_RATE_12_BITMAP;
            break;
        case SIR_MAC_RATE_18:
            rateBitmap |= SIR_MAC_RATE_18_BITMAP;
            break;
        case SIR_MAC_RATE_24:
            rateBitmap |= SIR_MAC_RATE_24_BITMAP;
            break;
        case SIR_MAC_RATE_36:
            rateBitmap |= SIR_MAC_RATE_36_BITMAP;
            break;
        case SIR_MAC_RATE_48:
            rateBitmap |= SIR_MAC_RATE_48_BITMAP;
            break;
        case SIR_MAC_RATE_54:
            rateBitmap |= SIR_MAC_RATE_54_BITMAP;
            break;
    }
    *pRateBitmap = rateBitmap;
}

tANI_BOOLEAN csrCheckRateBitmap(tANI_U8 rate, tANI_U16 rateBitmap)
{
    tANI_U16 n = BITS_OFF( rate, CSR_DOT11_BASIC_RATE_MASK );

    switch(n)
    {
        case SIR_MAC_RATE_1:
            rateBitmap &= SIR_MAC_RATE_1_BITMAP;
            break;
        case SIR_MAC_RATE_2:
            rateBitmap &= SIR_MAC_RATE_2_BITMAP;
            break;
        case SIR_MAC_RATE_5_5:
            rateBitmap &= SIR_MAC_RATE_5_5_BITMAP;
            break;
        case SIR_MAC_RATE_11:
            rateBitmap &= SIR_MAC_RATE_11_BITMAP;
            break;
        case SIR_MAC_RATE_6:
            rateBitmap &= SIR_MAC_RATE_6_BITMAP;
            break;
        case SIR_MAC_RATE_9:
            rateBitmap &= SIR_MAC_RATE_9_BITMAP;
            break;
        case SIR_MAC_RATE_12:
            rateBitmap &= SIR_MAC_RATE_12_BITMAP;
            break;
        case SIR_MAC_RATE_18:
            rateBitmap &= SIR_MAC_RATE_18_BITMAP;
            break;
        case SIR_MAC_RATE_24:
            rateBitmap &= SIR_MAC_RATE_24_BITMAP;
            break;
        case SIR_MAC_RATE_36:
            rateBitmap &= SIR_MAC_RATE_36_BITMAP;
            break;
        case SIR_MAC_RATE_48:
            rateBitmap &= SIR_MAC_RATE_48_BITMAP;
            break;
        case SIR_MAC_RATE_54:
            rateBitmap &= SIR_MAC_RATE_54_BITMAP;
            break;
    }
    return !!rateBitmap;
}

tANI_BOOLEAN csrRatesIsDot11RateSupported( tHalHandle hHal, tANI_U8 rate )
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    tANI_U16 n = BITS_OFF( rate, CSR_DOT11_BASIC_RATE_MASK );

    return csrIsAggregateRateSupported( pMac, n );
}


tANI_U16 csrRatesMacPropToDot11( tANI_U16 Rate )
{
    tANI_U16 ConvertedRate = Rate;

    switch( Rate )
    {
        case SIR_MAC_RATE_1:
            ConvertedRate = 2;
            break;
        case SIR_MAC_RATE_2:
            ConvertedRate = 4;
            break;
        case SIR_MAC_RATE_5_5:
            ConvertedRate = 11;
            break;
        case SIR_MAC_RATE_11:
            ConvertedRate = 22;
            break;

        case SIR_MAC_RATE_6:
            ConvertedRate = 12;
            break;
        case SIR_MAC_RATE_9:
            ConvertedRate = 18;
            break;
        case SIR_MAC_RATE_12:
            ConvertedRate = 24;
            break;
        case SIR_MAC_RATE_18:
            ConvertedRate = 36;
            break;
        case SIR_MAC_RATE_24:
            ConvertedRate = 48;
            break;
        case SIR_MAC_RATE_36:
            ConvertedRate = 72;
            break;
        case SIR_MAC_RATE_42:
            ConvertedRate = 84;
            break;
        case SIR_MAC_RATE_48:
            ConvertedRate = 96;
            break;
        case SIR_MAC_RATE_54:
            ConvertedRate = 108;
            break;

        case SIR_MAC_RATE_72:
            ConvertedRate = 144;
            break;
        case SIR_MAC_RATE_84:
            ConvertedRate = 168;
            break;
        case SIR_MAC_RATE_96:
            ConvertedRate = 192;
            break;
        case SIR_MAC_RATE_108:
            ConvertedRate = 216;
            break;
        case SIR_MAC_RATE_126:
            ConvertedRate = 252;
            break;
        case SIR_MAC_RATE_144:
            ConvertedRate = 288;
            break;
        case SIR_MAC_RATE_168:
            ConvertedRate = 336;
            break;
        case SIR_MAC_RATE_192:
            ConvertedRate = 384;
            break;
        case SIR_MAC_RATE_216:
            ConvertedRate = 432;
            break;
        case SIR_MAC_RATE_240:
            ConvertedRate = 480;
            break;

        case 0xff:
            ConvertedRate = 0;
            break;
    }

    return ConvertedRate;
}


tANI_U16 csrRatesFindBestRate( tSirMacRateSet *pSuppRates, tSirMacRateSet *pExtRates, tSirMacPropRateSet *pPropRates )
{
    tANI_U8 i;
    tANI_U16 nBest;

    nBest = pSuppRates->rate[ 0 ] & ( ~CSR_DOT11_BASIC_RATE_MASK );

    if(pSuppRates->numRates > SIR_MAC_RATESET_EID_MAX)
    {
        pSuppRates->numRates = SIR_MAC_RATESET_EID_MAX;
    }

    for ( i = 1U; i < pSuppRates->numRates; ++i )
    {
        nBest = (tANI_U16)CSR_MAX( nBest, pSuppRates->rate[ i ] & ( ~CSR_DOT11_BASIC_RATE_MASK ) );
    }

    if ( NULL != pExtRates )
    {
        for ( i = 0U; i < pExtRates->numRates; ++i )
        {
            nBest = (tANI_U16)CSR_MAX( nBest, pExtRates->rate[ i ] & ( ~CSR_DOT11_BASIC_RATE_MASK ) );
        }
    }

    if ( NULL != pPropRates )
    {
        for ( i = 0U; i < pPropRates->numPropRates; ++i )
        {
            nBest = (tANI_U16)CSR_MAX( nBest,  csrRatesMacPropToDot11( pPropRates->propRate[ i ] ) );
        }
    }

    return nBest;
}

#ifdef WLAN_FEATURE_FILS_SK
static inline void csr_free_fils_profile_info(tCsrRoamProfile *profile)
{
    if (profile->fils_con_info) {
        vos_mem_free(profile->fils_con_info);
        profile->fils_con_info = NULL;
    }
}
#else
static inline void csr_free_fils_profile_info(tCsrRoamProfile *profile)
{ }
#endif


void csrReleaseProfile(tpAniSirGlobal pMac, tCsrRoamProfile *pProfile)
{
    if(pProfile)
    {
        if(pProfile->BSSIDs.bssid)
        {
            vos_mem_free(pProfile->BSSIDs.bssid);
            pProfile->BSSIDs.bssid = NULL;
        }
        if(pProfile->SSIDs.SSIDList)
        {
            vos_mem_free(pProfile->SSIDs.SSIDList);
            pProfile->SSIDs.SSIDList = NULL;
        }
        if(pProfile->pWPAReqIE)
        {
            vos_mem_free(pProfile->pWPAReqIE);
            pProfile->pWPAReqIE = NULL;
        }
        if(pProfile->pRSNReqIE)
        {
            vos_mem_free(pProfile->pRSNReqIE);
            pProfile->pRSNReqIE = NULL;
        }
#ifdef FEATURE_WLAN_WAPI
        if(pProfile->pWAPIReqIE)
        {
            vos_mem_free(pProfile->pWAPIReqIE);
            pProfile->pWAPIReqIE = NULL;
        }
#endif /* FEATURE_WLAN_WAPI */

        if(pProfile->pAddIEScan)
        {
            vos_mem_free(pProfile->pAddIEScan);
            pProfile->pAddIEScan = NULL;
        }

        if(pProfile->pAddIEAssoc)
        {
            vos_mem_free(pProfile->pAddIEAssoc);
            pProfile->pAddIEAssoc = NULL;
        }
        if(pProfile->ChannelInfo.ChannelList)
        {
            vos_mem_free(pProfile->ChannelInfo.ChannelList);
            pProfile->ChannelInfo.ChannelList = NULL;
        }
        csr_free_fils_profile_info(pProfile);
        vos_mem_set(pProfile, sizeof(tCsrRoamProfile), 0);
    }
}

void csrFreeScanFilter(tpAniSirGlobal pMac, tCsrScanResultFilter *pScanFilter)
{
    if(pScanFilter->BSSIDs.bssid)
    {
        vos_mem_free(pScanFilter->BSSIDs.bssid);
        pScanFilter->BSSIDs.bssid = NULL;
    }
    if(pScanFilter->ChannelInfo.ChannelList)
    {
        vos_mem_free(pScanFilter->ChannelInfo.ChannelList);
        pScanFilter->ChannelInfo.ChannelList = NULL;
    }
    if(pScanFilter->SSIDs.SSIDList)
    {
        vos_mem_free(pScanFilter->SSIDs.SSIDList);
        pScanFilter->SSIDs.SSIDList = NULL;
    }
}


void csrFreeRoamProfile(tpAniSirGlobal pMac, tANI_U32 sessionId)
{
    tCsrRoamSession *pSession = &pMac->roam.roamSession[sessionId];

    if(pSession->pCurRoamProfile)
    {
        csrReleaseProfile(pMac, pSession->pCurRoamProfile);
        vos_mem_free(pSession->pCurRoamProfile);
        pSession->pCurRoamProfile = NULL;
    }
}


void csrFreeConnectBssDesc(tpAniSirGlobal pMac, tANI_U32 sessionId)
{
    tCsrRoamSession *pSession = &pMac->roam.roamSession[sessionId];

    if(pSession->pConnectBssDesc)
    {
        vos_mem_free(pSession->pConnectBssDesc);
        pSession->pConnectBssDesc = NULL;
    }
}



tSirResultCodes csrGetDisassocRspStatusCode( tSirSmeDisassocRsp *pSmeDisassocRsp )
{
    tANI_U8 *pBuffer = (tANI_U8 *)pSmeDisassocRsp;
    tANI_U32 ret;

    pBuffer += (sizeof(tANI_U16) + sizeof(tANI_U16) + sizeof(tSirMacAddr));
    /* tSirResultCodes is an enum, assuming is 32bit
       If we cannot make this assumption, use copy memory */
    pal_get_U32( pBuffer, &ret );

    return( ( tSirResultCodes )ret );
}


tSirResultCodes csrGetDeAuthRspStatusCode( tSirSmeDeauthRsp *pSmeRsp )
{
    tANI_U8 *pBuffer = (tANI_U8 *)pSmeRsp;
    tANI_U32 ret;

    pBuffer += (sizeof(tANI_U16) + sizeof(tANI_U16) + sizeof(tANI_U8) +sizeof(tANI_U16));
    /* tSirResultCodes is an enum, assuming is 32bit
       If we cannot make this assumption, use copy memory */
    pal_get_U32( pBuffer, &ret );

    return( ( tSirResultCodes )ret );
}

tSirScanType csrGetScanType(tpAniSirGlobal pMac, tANI_U8 chnId)
{
    tSirScanType scanType = eSIR_PASSIVE_SCAN;
    eNVChannelEnabledType channelEnabledType;

    channelEnabledType = vos_nv_getChannelEnabledState(chnId);
    if( NV_CHANNEL_ENABLE ==  channelEnabledType)
    {
         scanType = eSIR_ACTIVE_SCAN;
    }
    return (scanType);
}


tANI_U8 csrToUpper( tANI_U8 ch )
{
    tANI_U8 chOut;

    if ( ch >= 'a' && ch <= 'z' )
    {
        chOut = ch - 'a' + 'A';
    }
    else
    {
        chOut = ch;
    }
    return( chOut );
}


tSirBssType csrTranslateBsstypeToMacType(eCsrRoamBssType csrtype)
{
    tSirBssType ret;

    switch(csrtype)
    {
    case eCSR_BSS_TYPE_INFRASTRUCTURE:
        ret = eSIR_INFRASTRUCTURE_MODE;
        break;
    case eCSR_BSS_TYPE_IBSS:
    case eCSR_BSS_TYPE_START_IBSS:
        ret = eSIR_IBSS_MODE;
        break;
    case eCSR_BSS_TYPE_WDS_AP:
        ret = eSIR_BTAMP_AP_MODE;
        break;
    case eCSR_BSS_TYPE_WDS_STA:
        ret = eSIR_BTAMP_STA_MODE;
        break;
    case eCSR_BSS_TYPE_INFRA_AP:
        ret = eSIR_INFRA_AP_MODE;
        break;
    case eCSR_BSS_TYPE_NDI:
        ret = eSIR_NDI_MODE;
        break;
    case eCSR_BSS_TYPE_ANY:
    default:
        ret = eSIR_AUTO_MODE;
        break;
    }

    return (ret);
}


//This function use the parameters to decide the CFG value.
//CSR never sets WNI_CFG_DOT11_MODE_ALL to the CFG
//So PE should not see WNI_CFG_DOT11_MODE_ALL when it gets the CFG value
eCsrCfgDot11Mode csrGetCfgDot11ModeFromCsrPhyMode(tCsrRoamProfile *pProfile, eCsrPhyMode phyMode, tANI_BOOLEAN fProprietary)
{
    tANI_U32 cfgDot11Mode = eCSR_CFG_DOT11_MODE_ABG;

    switch(phyMode)
    {
    case eCSR_DOT11_MODE_11a:
        cfgDot11Mode = eCSR_CFG_DOT11_MODE_11A;
        break;
    case eCSR_DOT11_MODE_11b:
    case eCSR_DOT11_MODE_11b_ONLY:
        cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
        break;
    case eCSR_DOT11_MODE_11g:
    case eCSR_DOT11_MODE_11g_ONLY:
        if(pProfile && (CSR_IS_INFRA_AP(pProfile)) && (phyMode == eCSR_DOT11_MODE_11g_ONLY))
            cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G_ONLY;
        else
        cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
        break;
    case eCSR_DOT11_MODE_11n:
        cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
        break;
    case eCSR_DOT11_MODE_11n_ONLY:
       if(pProfile && CSR_IS_INFRA_AP(pProfile))
           cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N_ONLY;
       else
       cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
       break;
    case eCSR_DOT11_MODE_abg:
        cfgDot11Mode = eCSR_CFG_DOT11_MODE_ABG;
        break;
    case eCSR_DOT11_MODE_AUTO:
        cfgDot11Mode = eCSR_CFG_DOT11_MODE_AUTO;
        break;

#ifdef WLAN_FEATURE_11AC
    case eCSR_DOT11_MODE_11ac:
        if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
        {
            cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
        }
        else
        {
            cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
        }
        break;
    case eCSR_DOT11_MODE_11ac_ONLY:
        if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
        {
            cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC_ONLY;
        }
        else
        {
            cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
        }
        break;
#endif
    default:
        //No need to assign anything here
        break;
    }

    return (cfgDot11Mode);
}


eHalStatus csrSetRegulatoryDomain(tpAniSirGlobal pMac, v_REGDOMAIN_t domainId, tANI_BOOLEAN *pfRestartNeeded)
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    tANI_BOOLEAN fRestart;

    if(pMac->scan.domainIdCurrent == domainId)
    {
        //no change
        fRestart = eANI_BOOLEAN_FALSE;
    }
    else if( !pMac->roam.configParam.fEnforceDefaultDomain )
    {
        pMac->scan.domainIdCurrent = domainId;
        fRestart = eANI_BOOLEAN_TRUE;
    }
    else
    {
        //We cannot change the domain
        status = eHAL_STATUS_CSR_WRONG_STATE;
        fRestart = eANI_BOOLEAN_FALSE;
    }
    if(pfRestartNeeded)
    {
        *pfRestartNeeded = fRestart;
    }

    return (status);
}


v_REGDOMAIN_t csrGetCurrentRegulatoryDomain(tpAniSirGlobal pMac)
{
    return (pMac->scan.domainIdCurrent);
}


eHalStatus csrGetRegulatoryDomainForCountry
(
tpAniSirGlobal pMac,
tANI_U8 *pCountry,
v_REGDOMAIN_t *pDomainId,
v_CountryInfoSource_t source
)
{
    eHalStatus status = eHAL_STATUS_INVALID_PARAMETER;
    VOS_STATUS vosStatus;
    v_COUNTRYCODE_t countryCode;
    v_REGDOMAIN_t domainId;

    if(pCountry)
    {
        countryCode[0] = pCountry[0];
        countryCode[1] = pCountry[1];
        vosStatus = vos_nv_getRegDomainFromCountryCode(&domainId,
                                                       countryCode,
                                                       source);

        if( VOS_IS_STATUS_SUCCESS(vosStatus) )
        {
            if( pDomainId )
            {
                *pDomainId = domainId;
            }
            status = eHAL_STATUS_SUCCESS;
        }
        else
        {
            smsLog(pMac, LOGW, FL(" Couldn't find domain for country code  %c%c"), pCountry[0], pCountry[1]);
            status = eHAL_STATUS_INVALID_PARAMETER;
        }
    }

    return (status);
}

//To check whether a country code matches the one in the IE
//Only check the first two characters, ignoring in/outdoor
//pCountry -- caller allocated buffer contain the country code that is checking against
//the one in pIes. It can be NULL.
//caller must provide pIes, it cannot be NULL
//This function always return TRUE if 11d support is not turned on.
tANI_BOOLEAN csrMatchCountryCode( tpAniSirGlobal pMac, tANI_U8 *pCountry, tDot11fBeaconIEs *pIes )
{
    tANI_BOOLEAN fRet = eANI_BOOLEAN_TRUE;
    v_REGDOMAIN_t domainId = REGDOMAIN_COUNT;   //This is init to invalid value
    eHalStatus status;

    do
    {
        if( !csrIs11dSupported( pMac) )
        {
            break;
        }
        if( !pIes )
        {
            smsLog(pMac, LOGE, FL("  No IEs"));
            break;
        }
        if( pMac->roam.configParam.fEnforceDefaultDomain ||
            pMac->roam.configParam.fEnforceCountryCodeMatch )
        {
            //Make sure this country is recognizable
            if( pIes->Country.present )
            {
                status = csrGetRegulatoryDomainForCountry(pMac,
                                           pIes->Country.country,
                                           &domainId, COUNTRY_QUERY);
                if( !HAL_STATUS_SUCCESS( status ) )
                {
                     status = csrGetRegulatoryDomainForCountry(pMac,
                                                 pMac->scan.countryCode11d,
                                                 (v_REGDOMAIN_t *) &domainId,
                                                 COUNTRY_QUERY);
                     if( !HAL_STATUS_SUCCESS( status ) )
                     {
                           fRet = eANI_BOOLEAN_FALSE;
                           break;
                     }
                }
            }
            //check whether it is needed to enforce to the default regulatory domain first
            if( pMac->roam.configParam.fEnforceDefaultDomain )
            {
                if( domainId != pMac->scan.domainIdCurrent )
                {
                    fRet = eANI_BOOLEAN_FALSE;
                    break;
                }
            }
            if( pMac->roam.configParam.fEnforceCountryCodeMatch )
            {
            if( domainId >= REGDOMAIN_COUNT )
                {
                    fRet = eANI_BOOLEAN_FALSE;
                    break;
                }
            }
        }
        if( pCountry )
        {
            tANI_U32 i;

            if( !pIes->Country.present )
            {
                fRet = eANI_BOOLEAN_FALSE;
                break;
            }
            // Convert the CountryCode characters to upper
            for ( i = 0; i < WNI_CFG_COUNTRY_CODE_LEN - 1; i++ )
            {
                pCountry[i] = csrToUpper( pCountry[i] );
            }
            if (!vos_mem_compare(pIes->Country.country, pCountry,
                                WNI_CFG_COUNTRY_CODE_LEN - 1))
            {
                fRet = eANI_BOOLEAN_FALSE;
                break;
            }
        }
    } while(0);

    return (fRet);
}


eHalStatus csrGetModifyProfileFields(tpAniSirGlobal pMac, tANI_U32 sessionId,
                                     tCsrRoamModifyProfileFields *pModifyProfileFields)
{

   if(!pModifyProfileFields)
   {
      return eHAL_STATUS_FAILURE;
   }

   vos_mem_copy(pModifyProfileFields,
                &pMac->roam.roamSession[sessionId].connectedProfile.modifyProfileFields,
                sizeof(tCsrRoamModifyProfileFields));

   return eHAL_STATUS_SUCCESS;
}

eHalStatus csrSetModifyProfileFields(tpAniSirGlobal pMac, tANI_U32 sessionId,
                                     tCsrRoamModifyProfileFields *pModifyProfileFields)
{
   tCsrRoamSession *pSession = CSR_GET_SESSION( pMac, sessionId );

   vos_mem_copy(&pSession->connectedProfile.modifyProfileFields,
                 pModifyProfileFields,
                 sizeof(tCsrRoamModifyProfileFields));

   return eHAL_STATUS_SUCCESS;
}



/* ---------------------------------------------------------------------------
    \fn csrGetSupportedCountryCode
    \brief this function is to get a list of the country code current being supported
    \param pBuf - Caller allocated buffer with at least 3 bytes, upon success return,
    this has the country code list. 3 bytes for each country code. This may be NULL if
    caller wants to know the needed bytes.
    \param pbLen - Caller allocated, as input, it indicates the length of pBuf. Upon success return,
    this contains the length of the data in pBuf
    \return eHalStatus
  -------------------------------------------------------------------------------*/
eHalStatus csrGetSupportedCountryCode(tpAniSirGlobal pMac, tANI_U8 *pBuf, tANI_U32 *pbLen)
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    VOS_STATUS vosStatus;
    v_SIZE_t size = (v_SIZE_t)*pbLen;

    vosStatus = vos_nv_getSupportedCountryCode( pBuf, &size, 1 );
    /* Either way, return the value back */
    *pbLen = (tANI_U32)size;

    //If pBuf is NULL, caller just want to get the size, consider it success
    if(pBuf)
    {
        if( VOS_IS_STATUS_SUCCESS( vosStatus ) )
        {
            tANI_U32 i, n = *pbLen / 3;

            for( i = 0; i < n; i++ )
            {
                pBuf[i*3 + 2] = ' ';
            }
        }
        else
        {
            status = eHAL_STATUS_FAILURE;
        }
    }

    return (status);
}



//Upper layer to get the list of the base channels to scan for passively 11d info from csr
eHalStatus csrScanGetBaseChannels( tpAniSirGlobal pMac, tCsrChannelInfo * pChannelInfo )
{
    eHalStatus status = eHAL_STATUS_FAILURE;

    do
    {

       if(!pMac->scan.baseChannels.numChannels || !pChannelInfo)
       {
          break;
       }
       pChannelInfo->ChannelList = vos_mem_malloc(pMac->scan.baseChannels.numChannels);
       if ( NULL == pChannelInfo->ChannelList )
       {
          smsLog( pMac, LOGE, FL("csrScanGetBaseChannels: fail to allocate memory") );
          return eHAL_STATUS_FAILURE;
       }
       vos_mem_copy(pChannelInfo->ChannelList,
                    pMac->scan.baseChannels.channelList,
                    pMac->scan.baseChannels.numChannels);
       pChannelInfo->numOfChannels = pMac->scan.baseChannels.numChannels;

    }while(0);

    return ( status );
}


tANI_BOOLEAN csrIsSetKeyAllowed(tpAniSirGlobal pMac, tANI_U32 sessionId)
{
    tANI_BOOLEAN fRet = eANI_BOOLEAN_TRUE;
    tCsrRoamSession *pSession;

    pSession =CSR_GET_SESSION(pMac, sessionId);

    /*This condition is not working for infra state. When infra is in not-connected state
    * the pSession->pCurRoamProfile is NULL. And this function returns TRUE, that is incorrect.
    * Since SAP requires to set key without any BSS started, it needs this condition to be met.
    * In other words, this function is useless.
    * The current work-around is to process setcontext_rsp and removekey_rsp no matter what the
    * state is.
    */
    smsLog( pMac, LOG2, FL(" is not what it intends to. Must be revisit or removed") );
    if( (NULL == pSession) ||
        ( csrIsConnStateDisconnected( pMac, sessionId ) &&
        (pSession->pCurRoamProfile != NULL) &&
        (!(CSR_IS_INFRA_AP(pSession->pCurRoamProfile))) )
        )
    {
        fRet = eANI_BOOLEAN_FALSE;
    }

    return ( fRet );
}

//no need to acquire lock for this basic function
tANI_U16 sme_ChnToFreq(tANI_U8 chanNum)
{
   int i;

   for (i = 0; i < NUM_RF_CHANNELS; i++)
   {
      if (rfChannels[i].channelNum == chanNum)
      {
         return rfChannels[i].targetFreq;
      }
   }

   return (0);
}

/* Disconnect all active sessions by sending disassoc. This is mainly used to disconnect the remaining session when we
 * transition from concurrent sessions to a single session. The use case is Infra STA and wifi direct multiple sessions are up and
 * P2P session is removed. The Infra STA session remains and should resume BMPS if BMPS is enabled by default. However, there
 * are some issues seen with BMPS resume during this transition and this is a workaround which will allow the Infra STA session to
 * disconnect and auto connect back and enter BMPS this giving the same effect as resuming BMPS
 */

//Remove this code once SLM_Sessionization is supported
//BMPS_WORKAROUND_NOT_NEEDED
void csrDisconnectAllActiveSessions(tpAniSirGlobal pMac)
{
    tANI_U8 i;

    /* Disconnect all the active sessions */
    for (i=0; i<CSR_ROAM_SESSION_MAX; i++)
    {
        if( CSR_IS_SESSION_VALID( pMac, i ) && !csrIsConnStateDisconnected( pMac, i ) )
        {
            csrRoamDisconnectInternal(pMac, i, eCSR_DISCONNECT_REASON_UNSPECIFIED);
        }
    }
}

/**
 * csr_get_channel_status() - get chan info via channel number
 * @p_mac: Pointer to Global MAC structure
 * @channel_id: channel id
 *
 * Return: chan status info
 */
struct lim_channel_status *csr_get_channel_status(
	void *p_mac, uint32_t channel_id)
{
	uint8_t i;
	struct lim_scan_channel_status *channel_status;
	tpAniSirGlobal mac_ptr = (tpAniSirGlobal)p_mac;

	if (ACS_FW_REPORT_PARAM_CONFIGURED) {
		channel_status = (struct lim_scan_channel_status *)
				&mac_ptr->lim.scan_channel_status;
		for (i = 0; i < channel_status->total_channel; i++) {
			if (channel_status->channel_status_list[i].channel_id
				 == channel_id)
				return &channel_status->channel_status_list[i];
		}
		smsLog(mac_ptr, LOGW,
			 FL("Channel %d status info not exist"),
			  channel_id);
	}
	return NULL;
}

/**
 * csr_clear_channel_status() - clear chan info
 * @p_mac: Pointer to Global MAC structure
 *
 * Return: none
 */
void csr_clear_channel_status(void *p_mac)
{
	tpAniSirGlobal mac_ptr = (tpAniSirGlobal)p_mac;
	struct lim_scan_channel_status *channel_status;
	if (ACS_FW_REPORT_PARAM_CONFIGURED) {
		channel_status = (struct lim_scan_channel_status *)
				&mac_ptr->lim.scan_channel_status;
		channel_status->total_channel = 0;
	}
	return;
}

#ifdef FEATURE_WLAN_LFR
tANI_BOOLEAN csrIsChannelPresentInList(
        tANI_U8 *pChannelList,
        int  numChannels,
        tANI_U8   channel
        )
{
    int i = 0;

    // Check for NULL pointer
    if (!pChannelList || (numChannels == 0))
    {
       return FALSE;
    }

    // Look for the channel in the list
    for (i = 0; (i < numChannels) &&
            (i < WNI_CFG_VALID_CHANNEL_LIST_LEN); i++)
    {
        if (pChannelList[i] == channel)
            return TRUE;
    }

    return FALSE;
}

/**
 * sme_requestTypetoString(): converts scan request enum to string.
 * @requestType: scan request type enum.
 */
const char * sme_requestTypetoString(const v_U8_t requestType)
{
    switch (requestType)
    {
        CASE_RETURN_STRING( eCSR_SCAN_REQUEST_11D_SCAN );
        CASE_RETURN_STRING( eCSR_SCAN_REQUEST_FULL_SCAN );
        CASE_RETURN_STRING( eCSR_SCAN_IDLE_MODE_SCAN );
        CASE_RETURN_STRING( eCSR_SCAN_HO_BG_SCAN );
        CASE_RETURN_STRING( eCSR_SCAN_HO_PROBE_SCAN );
        CASE_RETURN_STRING( eCSR_SCAN_HO_NT_BG_SCAN );
        CASE_RETURN_STRING( eCSR_SCAN_P2P_DISCOVERY );
        CASE_RETURN_STRING( eCSR_SCAN_SOFTAP_CHANNEL_RANGE );
        CASE_RETURN_STRING( eCSR_SCAN_P2P_FIND_PEER );
        default:
            return "Unknown Scan Request Type";
    }
}

/**
 * sme_scan_type_to_string() - converts scan type enum to string.
 * @scan_type: scan type enum
 *
 * Return: printable string for scan type
 */
const char * sme_scan_type_to_string(const uint8_t scan_type)
{
	switch (scan_type) {
	CASE_RETURN_STRING(eSIR_PASSIVE_SCAN);
	CASE_RETURN_STRING(eSIR_ACTIVE_SCAN);
	CASE_RETURN_STRING(eSIR_BEACON_TABLE);
	default:
		return "Unknown ScanType";
	}
}

/**
 * sme_bss_type_to_string() - converts bss type enum to string.
 * @bss_type: bss type enum
 *
 * Return: printable string for bss type
 */
const char * sme_bss_type_to_string(const uint8_t bss_type)
{
	switch (bss_type) {
	CASE_RETURN_STRING(eSIR_INFRASTRUCTURE_MODE);
	CASE_RETURN_STRING(eSIR_INFRA_AP_MODE);
	CASE_RETURN_STRING(eSIR_IBSS_MODE);
	CASE_RETURN_STRING(eSIR_BTAMP_STA_MODE);
	CASE_RETURN_STRING(eSIR_BTAMP_AP_MODE);
	CASE_RETURN_STRING(eSIR_AUTO_MODE);
	CASE_RETURN_STRING(eSIR_NDI_MODE);
	default:
		return "Unknown BssType";
	}
}

VOS_STATUS csrAddToChannelListFront(
        tANI_U8 *pChannelList,
        int  numChannels,
        tANI_U8   channel
        )
{
    int i = 0;

    // Check for NULL pointer
    if (!pChannelList) return eHAL_STATUS_E_NULL_VALUE;

    // Make room for the addition.  (Start moving from the back.)
    for (i = numChannels; i > 0; i--)
    {
        pChannelList[i] = pChannelList[i-1];
    }

    // Now add the NEW channel...at the front
    pChannelList[0] = channel;

    return eHAL_STATUS_SUCCESS;
}
#endif
#ifdef FEATURE_WLAN_DIAG_SUPPORT
/**
 * csr_diag_event_report() - send PE diag event
 * @pmac:        pointer to global MAC context.
 * @event_typev: sub event type for DIAG event.
 * @status:      status of the event
 * @reasoncode:  reasoncode for the given status
 *
 * This function is called to send diag event
 *
 * Return:   NA
 */

void csr_diag_event_report(tpAniSirGlobal pmac, uint16_t event_type,
			   uint16_t status, uint16_t reasoncode)
{
	tSirMacAddr nullbssid = { 0, 0, 0, 0, 0, 0 };
	WLAN_VOS_DIAG_EVENT_DEF(diag_event, vos_event_wlan_pe_payload_type);

	vos_mem_set(&diag_event, sizeof(vos_event_wlan_pe_payload_type), 0);

	vos_mem_copy(diag_event.bssid, nullbssid, sizeof(tSirMacAddr));
	diag_event.sme_state = (tANI_U16)pmac->lim.gLimSmeState;
	diag_event.mlm_state = (tANI_U16)pmac->lim.gLimMlmState;
	diag_event.event_type = event_type;
	diag_event.status = status;
	diag_event.reason_code = reasoncode;

	WLAN_VOS_DIAG_EVENT_REPORT(&diag_event, EVENT_WLAN_PE);
	return;
}
#endif
