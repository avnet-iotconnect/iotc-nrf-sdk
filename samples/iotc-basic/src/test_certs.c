#include <stdbool.h>
#include <string.h>

#include "nrf_cert_store.h"
#include "test_certs.h"

/**
 * This file contains an example of how you could program your a set of your
 * certificates and keys for a few test devices.
 *
 * This is NOT the recommended way to program your devices as it will contain
 * the keys anc certs in the the actual build binaries.
 *
 * Another way to program your keys is to use the LTE Link Monitor Nordic
 * application. It has a certificate manager application. The MQTT credentials
 * are stored in Security Tag is defined by TLS_SEC_TAG_IOTCONNECT_MQTT (10701)
 * in nrf_cert_store.h. TLS_SEC_TAG_IOTCONNECT_MQTT also requires
 * the Baltimore Cyber Trust (publicly available) as "CA Certificate" in addtion
 * the the device key and cert.
 *
 */

static const char *QA_PKEY_352656100380644 =
        "-----BEGIN EC PRIVATE KEY-----\r\n"
        "MHQCAQEEIJg7p8wbSoUCTCfm6Xopgo9F0TYEQsHkoq6+yuvmGo1SoAcGBSuBBAAK\r\n"
        "oUQDQgAEfeRTZCK1tAH2mCXqN+u4qfvZEVloxw1EQpXlU7973FjLNB+N74cQNsKp\r\n"
        "qPH33U6DHfDT2ZsyPHPaChc1fjpLlQ==\r\n"
        "-----END EC PRIVATE KEY-----\r\n";


static const char *QA_CERT_352656100380644 =
        "-----BEGIN CERTIFICATE-----\r\n"
        "MIIB8DCCAZYCAQEwCgYIKoZIzj0EAwIwcTELMAkGA1UEBhMCVVMxETAPBgNVBAgM\r\n"
        "CElsbGlub2lzMRAwDgYDVQQHDAdDaGljYWdvMRAwDgYDVQQKDAdOcmZEZW1vMRQw\r\n"
        "EgYDVQQLDAtOcmZEZW1vIE9yZzEVMBMGA1UEAwwMTnJmRGVtby1UZXN0MB4XDTIw\r\n"
        "MDgxMDE4MTMwOVoXDTQ3MTIyNjE4MTMwOVowgZkxCzAJBgNVBAYTAlVTMREwDwYD\r\n"
        "VQQIDAhJbGxpbm9pczEQMA4GA1UEBwwHQ2hpY2FnbzEQMA4GA1UECgwHTnJmRGVt\r\n"
        "bzEUMBIGA1UECwwLTnJmRGVtbyBPcmcxPTA7BgNVBAMMNFFVVTFOemN4TkRBdFFV\r\n"
        "WTRSQzAwUWpRNExVRTVOMEl0LW5yZi0zNTI2NTYxMDAzODA2NDQwVjAQBgcqhkjO\r\n"
        "PQIBBgUrgQQACgNCAAR95FNkIrW0AfaYJeo367ip+9kRWWjHDURCleVTv3vcWMs0\r\n"
        "H43vhxA2wqmo8ffdToMd8NPZmzI8c9oKFzV+OkuVMAoGCCqGSM49BAMCA0gAMEUC\r\n"
        "IQCXx2NxJD6JsbfDi8rQI/ip9y63ESBr9zJkQd3LrEH6QAIgd9Zcs+a3me6nNLd8\r\n"
        "Rh9nLC8ZkFxKHFDrBhd2bQfgX0c=\r\n"
        "-----END CERTIFICATE-----\r\n";

static const char *QA_PKEY_352656100880056 =
        "-----BEGIN EC PRIVATE KEY-----\r\n"
        "MHQCAQEEIHKUqf05pmF/c1DrGa+D32/Zak+slJFY93XGlqBu1eceoAcGBSuBBAAK\r\n"
        "oUQDQgAEyc19YmLr4nnUYV5yvn009Ow2ZLvFdBpRm4HilHewBDYmUkhXrpeauUW7\r\n"
        "iImHs/UX4//EqcB8neQjrIABfymYAA==\r\n"
        "-----END EC PRIVATE KEY-----\r\n";

// NK#1
static const char *QA_CERT_352656100880056 =
        "-----BEGIN CERTIFICATE-----\r\n"
        "MIIB8TCCAZYCAQEwCgYIKoZIzj0EAwIwcTELMAkGA1UEBhMCVVMxETAPBgNVBAgM\r\n"
        "CElsbGlub2lzMRAwDgYDVQQHDAdDaGljYWdvMRAwDgYDVQQKDAdOcmZEZW1vMRQw\r\n"
        "EgYDVQQLDAtOcmZEZW1vIE9yZzEVMBMGA1UEAwwMTnJmRGVtby1UZXN0MB4XDTIw\r\n"
        "MDgxMDE4MTMwOVoXDTQ3MTIyNjE4MTMwOVowgZkxCzAJBgNVBAYTAlVTMREwDwYD\r\n"
        "VQQIDAhJbGxpbm9pczEQMA4GA1UEBwwHQ2hpY2FnbzEQMA4GA1UECgwHTnJmRGVt\r\n"
        "bzEUMBIGA1UECwwLTnJmRGVtbyBPcmcxPTA7BgNVBAMMNFFVVTFOemN4TkRBdFFV\r\n"
        "WTRSQzAwUWpRNExVRTVOMEl0LW5yZi0zNTI2NTYxMDA4ODAwNTYwVjAQBgcqhkjO\r\n"
        "PQIBBgUrgQQACgNCAATJzX1iYuviedRhXnK+fTT07DZku8V0GlGbgeKUd7AENiZS\r\n"
        "SFeul5q5RbuIiYez9Rfj/8SpwHyd5COsgAF/KZgAMAoGCCqGSM49BAMCA0kAMEYC\r\n"
        "IQD0O/jTQHnfpVnPChp2+YagjiLqC0LbbEy7CHMVK8lI3wIhAJJUkcZxqHmBMEDL\r\n"
        "dDqXbVK098xolz+G5wan6S4p27OB\r\n"
        "-----END CERTIFICATE-----\r\n";

// TK#1
static const char *QA_PKEY_352656100878605 =
        "-----BEGIN EC PRIVATE KEY-----\r\n"
        "MHQCAQEEIOHIMJ04dxmvupQKoe3v+FqvPkenpHL4NaDmL6kaTkd3oAcGBSuBBAAK\r\n"
        "oUQDQgAEJ79o+SSkng7G9XDUqy4Tt3sCUxhAjKKff1GSxyp78oo/V1mfCZkWLVhU\r\n"
        "1VvagTCGyhyKh0adqunQuvgv3XoPIQ==\r\n"
        "-----END EC PRIVATE KEY-----\r\n";

static const char *QA_CERT_352656100878605 =
        "-----BEGIN CERTIFICATE-----\r\n"
        "MIIB8TCCAZYCAQEwCgYIKoZIzj0EAwIwcTELMAkGA1UEBhMCVVMxETAPBgNVBAgM\r\n"
        "CElsbGlub2lzMRAwDgYDVQQHDAdDaGljYWdvMRAwDgYDVQQKDAdOcmZEZW1vMRQw\r\n"
        "EgYDVQQLDAtOcmZEZW1vIE9yZzEVMBMGA1UEAwwMTnJmRGVtby1UZXN0MB4XDTIw\r\n"
        "MDgxMDE4MTMwOVoXDTQ3MTIyNjE4MTMwOVowgZkxCzAJBgNVBAYTAlVTMREwDwYD\r\n"
        "VQQIDAhJbGxpbm9pczEQMA4GA1UEBwwHQ2hpY2FnbzEQMA4GA1UECgwHTnJmRGVt\r\n"
        "bzEUMBIGA1UECwwLTnJmRGVtbyBPcmcxPTA7BgNVBAMMNFFVVTFOemN4TkRBdFFV\r\n"
        "WTRSQzAwUWpRNExVRTVOMEl0LW5yZi0zNTI2NTYxMDA4Nzg2MDUwVjAQBgcqhkjO\r\n"
        "PQIBBgUrgQQACgNCAAQnv2j5JKSeDsb1cNSrLhO3ewJTGECMop9/UZLHKnvyij9X\r\n"
        "WZ8JmRYtWFTVW9qBMIbKHIqHRp2q6dC6+C/deg8hMAoGCCqGSM49BAMCA0kAMEYC\r\n"
        "IQC/spakcGnJS8p+7IuxEeXp4vs5fOb/gbkZWTLF+FOPcAIhAPRORA2iKz9OpOmh\r\n"
        "OXIf+XELpauDSRvGHpDuYkPvxF8S\r\n"
        "-----END CERTIFICATE-----\r\n";

// TK#2
static const char *QA_PKEY_352656100877458 =
        "-----BEGIN EC PRIVATE KEY-----\r\n"
        "MHQCAQEEINpaSibzzB4kuPFuOufGiaRzjeVFus+aYeLkDLdBXJlSoAcGBSuBBAAK\r\n"
        "oUQDQgAETwzU14fFJEzYFA3WumsuKf6jok46db4B4iOcl3zS/GrwHzmcmg0vXiKF\r\n"
        "R7Gd8WiUCpersMs+9TrmG+upF4CJnw==\r\n"
        "-----END EC PRIVATE KEY-----\r\n";

static const char *QA_CERT_352656100877458 =
        "-----BEGIN CERTIFICATE-----\r\n"
        "MIIB8DCCAZYCAQEwCgYIKoZIzj0EAwIwcTELMAkGA1UEBhMCVVMxETAPBgNVBAgM\r\n"
        "CElsbGlub2lzMRAwDgYDVQQHDAdDaGljYWdvMRAwDgYDVQQKDAdOcmZEZW1vMRQw\r\n"
        "EgYDVQQLDAtOcmZEZW1vIE9yZzEVMBMGA1UEAwwMTnJmRGVtby1UZXN0MB4XDTIw\r\n"
        "MDgxMDE4MTMwOVoXDTQ3MTIyNjE4MTMwOVowgZkxCzAJBgNVBAYTAlVTMREwDwYD\r\n"
        "VQQIDAhJbGxpbm9pczEQMA4GA1UEBwwHQ2hpY2FnbzEQMA4GA1UECgwHTnJmRGVt\r\n"
        "bzEUMBIGA1UECwwLTnJmRGVtbyBPcmcxPTA7BgNVBAMMNFFVVTFOemN4TkRBdFFV\r\n"
        "WTRSQzAwUWpRNExVRTVOMEl0LW5yZi0zNTI2NTYxMDA4Nzc0NTgwVjAQBgcqhkjO\r\n"
        "PQIBBgUrgQQACgNCAARPDNTXh8UkTNgUDda6ay4p/qOiTjp1vgHiI5yXfNL8avAf\r\n"
        "OZyaDS9eIoVHsZ3xaJQKl6uwyz71OuYb66kXgImfMAoGCCqGSM49BAMCA0gAMEUC\r\n"
        "IFcejowHG6KkE5fNBSg7qhM7EFEiNiLoVFahOWNK87JcAiEAt7lsKt0irTpVVILS\r\n"
        "dPtxn389ifSveVaiyFNl+bc1S8U=\r\n"
        "-----END CERTIFICATE-----\r\n";

// TS#1
// nrf-352656100883605
static const char *QA_PKEY_352656100883605 =
        "-----BEGIN EC PRIVATE KEY-----\r\n"
        "MHQCAQEEIOnReXv11Aup2KAAoXBcG+j5hse2NTQKOBWnpC07nsRtoAcGBSuBBAAK\r\n"
        "oUQDQgAEpLe0tSCyUdbLB8L88ou9nmrKSqkhJL4f7+KcrxF1+RsONT9siH0273nl\r\n"
        "RnMuwOA3BBMHDBmenHh+aqO6D4eziw==\r\n"
        "-----END EC PRIVATE KEY-----\r\n";

static const char *QA_CERT_352656100883605 =
        "-----BEGIN CERTIFICATE-----\r\n"
        "MIIB8DCCAZYCAQEwCgYIKoZIzj0EAwIwcTELMAkGA1UEBhMCVVMxETAPBgNVBAgM\r\n"
        "CElsbGlub2lzMRAwDgYDVQQHDAdDaGljYWdvMRAwDgYDVQQKDAdOcmZEZW1vMRQw\r\n"
        "EgYDVQQLDAtOcmZEZW1vIE9yZzEVMBMGA1UEAwwMTnJmRGVtby1UZXN0MB4XDTIw\r\n"
        "MDgxMDE4MTMwOVoXDTQ3MTIyNjE4MTMwOVowgZkxCzAJBgNVBAYTAlVTMREwDwYD\r\n"
        "VQQIDAhJbGxpbm9pczEQMA4GA1UEBwwHQ2hpY2FnbzEQMA4GA1UECgwHTnJmRGVt\r\n"
        "bzEUMBIGA1UECwwLTnJmRGVtbyBPcmcxPTA7BgNVBAMMNFFVVTFOemN4TkRBdFFV\r\n"
        "WTRSQzAwUWpRNExVRTVOMEl0LW5yZi0zNTI2NTYxMDA4ODM2MDUwVjAQBgcqhkjO\r\n"
        "PQIBBgUrgQQACgNCAASkt7S1ILJR1ssHwvzyi72easpKqSEkvh/v4pyvEXX5Gw41\r\n"
        "P2yIfTbveeVGcy7A4DcEEwcMGZ6ceH5qo7oPh7OLMAoGCCqGSM49BAMCA0gAMEUC\r\n"
        "IEb6Brw3wYBiQ6GbL+AwGQNh3F0fOMUoEOJqoiNDbSsVAiEA4OAS/P2NmiczSBTY\r\n"
        "B+Fq/VAMVrNnam5EGXlAsvc2Gkw=\r\n"
        "-----END CERTIFICATE-----\r\n";


int program_test_certs(const char *env, const char *imei) {
    int err = 0;

    // in this example, we will hardocde some QA certificates in our provisioning build
    // but only if environment = "qa".
    if (0 == strcmp("qa", env)) {
        if (0 == strcmp("352656100380644", imei)) {
            err = nrf_cert_store_save_device_cert(
                    QA_PKEY_352656100380644,
                    QA_CERT_352656100380644
            );
        } else if (0 == strcmp("352656100880056", imei)) {
            err = nrf_cert_store_save_device_cert(
                    QA_PKEY_352656100880056,
                    QA_CERT_352656100880056
            );
        } else if (0 == strcmp("352656100878605", imei)) {
            err = nrf_cert_store_save_device_cert(
                    QA_PKEY_352656100878605,
                    QA_CERT_352656100878605
            );
        } else if (0 == strcmp("352656100877458", imei)) {
            err = nrf_cert_store_save_device_cert(
                    QA_PKEY_352656100877458,
                    QA_CERT_352656100877458
            );
        } else if (0 == strcmp("352656100883605", imei)) {
            err = nrf_cert_store_save_device_cert(
                    QA_PKEY_352656100883605,
                    QA_CERT_352656100883605
            );
        }
    }

    return err;
}