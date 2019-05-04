// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2016, 2019 - TortoiseGit

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "stdafx.h"
#define NEED_SIGNING_KEY
#include "../version.h"
#include "StringUtils.h"
#include "UpdateCrypto.h"

TEST(UpdateCrypto, SimpleVerify)
{
	CAutoTempDir tempdir;
	CString testFile = tempdir.GetTempDir() + L"\\.test.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"Text"));

	CString testFileSignature = testFile + L".rsa.asc";
	EXPECT_EQ(-1, VerifyIntegrity(testFile, testFileSignature, nullptr));

	CString signature = L"-----BEGIN PGP SIGNATURE-----\nVersion: GnuPG v2\n\niQIcBAEBCgAGBQJVxPizAAoJEPfxez+d2VOeHrgP/jOQwPGToQOVgpWixwrwhflL\nzK/hpgpBzgKALzPKfQcXd/iL2WvIldmMlGo9inNeEIh+a2ufrzsr/N/CzKqeTPkb\nazSF99LtRiIoFd3ZSf4JPpwUNyCzMn5OfOhsBDbU7vpyu4OCqSjsM4EdGynh6CdW\nLEiZ/25EZB/70ZGkTaQewx5wPi0Kmyv0M52b7TrKixxK8iuOjENviUNtvvpbvjUt\nVfv2lZV/lWkq5lesHFjg8HsDBcwC7LPQbR3M1MSW5wH6Kp/9OPifJh5xkRvRGsr2\nEucLGXKoRZXm8AxFYZ6diN0vai3oQP7nUNEUURZLG9yEQ4kCfoArUwtRvNrqJrrx\nkH57hM9Zft1Yxkj48USxJmi9reM7QahPsUqhWNnb3KUq+752OaOU3Sxd7ZW6YHzk\nSMvwcp0Jj8KczHtMyI4Nhj8CkzBgOs6Bi3ydoXCozGsxcrv6zCs2v1je3CeSP7rM\nK4f6dwstExPGtm46TU4L05EIJYjhneI7EUcgPKgJ0c7AdtaOcwLDZdEjmaDFH83N\nbSJ8Rune2uXwIDFX7BpZSSbSedE5yKt/o7sgnpoFOMHRkRCxjRD/lFgj9c0nlxm2\nFgHYyEM+1o1Ps9vKEZMbmpkzxTV4ZOPaiD/ZUzq1s9yx58Ja6zmVcIaZw79JEhiP\nOKnABUyX8A4V9bPLkQ1M\n=PI7W\n-----END PGP SIGNATURE-----";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFileSignature, signature));
	EXPECT_EQ(0, VerifyIntegrity(testFile, testFileSignature, nullptr));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFileSignature, signature.Left(50)));
	EXPECT_EQ(-1, VerifyIntegrity(testFile, testFileSignature, nullptr));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"Text\n"));
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFileSignature, signature));
	EXPECT_EQ(-1, VerifyIntegrity(testFile, testFileSignature, nullptr));
}

TEST(UpdateCrypto, DisableOldCrypto)
{
	CAutoTempDir tempdir;
	CString testFile = tempdir.GetTempDir() + L"\\.test.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"Text"));

	CString signatureSHA1 = L"-----BEGIN PGP SIGNATURE-----\n\niQIzBAEBAgAdFiEEdKIa4wGzylvYBy9e9/F7P53ZU54FAlzN3/wACgkQ9/F7P53Z\nU57Z+w//d0JbWPOD9RWf+MkXmug9ftfL2JIZrQbOsnV5zMoVBz5yELCdfUCyD2FZ\np8RvyGzyjvEJfvKxfJpnk1dzc18QYceCIO3fB3lJXmGzmK1s7TRm14DuU17kSVeq\nVvYCOdPod1qEezgLwVltnh9NoCdtbWI4un3YMmG4/+vJ1QVgFp1txlYInDx13EaP\nbbtIa+LGCtTxSWMZI+AsdUIPY/Bh3KKBwaFUApFczNhz23J2t0DgvLmUIbuzOeeO\nHKq5V9Cbf5QQdS+8HQ08Isrv6/8aQrM1WLabgMPhRm4s0Yeq7TJEULzeU+r/jc7T\nju9xsrnAcMb1PGHh2pnQ0LuXnjvPAS2XvVLzTwF1B9ZUCCF11jqpB/Dg1nG26OYH\n1g+oCfgwbapVhuAEkEqScfT0B+/HOyTp6ghqgTN0K0mShDG+lXXRi6UGY9qdC01Z\n633HjljIAm9FKLHQW+OnWeK9eJh5A3/7k/M8UAF+5teux2k18vSklI4WSokcOCt/\nq+Zk6T/a+nUKzCGNGIjFPcdhvpgDxaGxECT3yw6Lv/RJTx/AAT6weWsLpKrGzOHd\nnhpOQUZ10og4K0FZrTL1LRx8+LWsMIKcDyxzNynDw7KfIX+OpkkywgSM49WMpajh\nCs6nVcKeijRkjhfxeQmnIU094bZ8dAt+tIm82BvUjB6bDeEkTg4=\n=Y+Xk\n-----END PGP SIGNATURE-----";
	CString testFileSignature = testFile + L".rsa.asc";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFileSignature, signatureSHA1));
	EXPECT_EQ(-1, VerifyIntegrity(testFile, testFileSignature, nullptr));
}

TEST(UpdateCrypto, NewLine)
{
	CAutoTempDir tempdir;
	CString testFile = tempdir.GetTempDir() + L"\\.test.txt";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"Text with\nNewline"));

	CString testFileSignature = testFile + L".rsa.asc";
	CString signature = L"-----BEGIN PGP SIGNATURE-----\nVersion: GnuPG v2\n\niQIcBAEBCgAGBQJVxPp9AAoJEPfxez+d2VOeGsIQAJYSxXcBeYs7gwz3+NwWSv/D\nUL+d1rJgIwyIXQ46BjiCno0a6NJ7JxPYkhl9Tqj2WenQRzQqqYZNRUPN8xEU0lSC\nS8q2B+uEGopqHG5a+/X/FJh1ijeeK75ey3HKHyn7BpH66IsDU2HZ517w6/FJOsHE\n295s+RwgojR7ghk+dqu+LrkJxldQ3r9jRDujuez5NSj1aPU5IRazyf864vl13W9O\n+7NfyjNyPmZxK/nzYPi9nujsWGXjVKYqomzfEUu7FShv4Ic0vdzHcSQdS4wVsxHc\n9Feo1dfTTaCrXnYo5vSQB+b4C6EQ0DZXW0vGOjwBFHpHCLuAgyOaUccnsyKZDNnx\nmx8QK1hb13gP5oBsShS5VxgBGhl3BP47jTiaSIf9QSGOGZih+kfAzcu0e2VGJ5Xo\nYkTAhAH/x9kRjCRze0qtH7Y3CIY3O/fFTnQmNVGr25Wrhfs7aL0rF80mTk/lwbR+\nGIlrRC2lyQGEl8hfgHfvnau+biW0GRKIINLB/i86VFzUwPS5Y4XoAbKftQoEdyQj\nJ1OaVDQxlIvx/6O0bY2nw/9fflnoeBFWzcgyaYOAV95XwVjid4DkQyInkoLGCC1p\novO/2+3zPnOoab/L8ZlR8lzgDBjn5zzZRxX0CHlgLPPzLpTCFZgxOh8KIzJ04uKU\nY72YA8friiI836RZVoXq\n=i5FK\n-----END PGP SIGNATURE-----";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFileSignature, signature));
	EXPECT_EQ(0, VerifyIntegrity(testFile, testFileSignature, nullptr));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"Text with\r\nNewline"));
	EXPECT_EQ(0, VerifyIntegrity(testFile, testFileSignature, nullptr));

	EXPECT_EQ(16, signature.Replace(L"\n", L"\r\n"));
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFileSignature, signature));
	EXPECT_EQ(0, VerifyIntegrity(testFile, testFileSignature, nullptr));
}

TEST(UpdateCrypto, Binary)
{
	CAutoTempDir tempdir;
	CString testFile = tempdir.GetTempDir() + L"\\test.bin";

	unsigned char binaryFile[99] = // based on link.png
	{
		0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
		0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x01, 0x03, 0x00, 0x00, 0x00, 0xFE, 0xC1, 0x2C,
		0xC8, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4C, 0x54, 0x45, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xA5,
		0xD9, 0x9F, 0xDD, 0x00, 0x00, 0x00, 0x18, 0x49, 0x44, 0x41, 0x54, 0x78, 0x5E, 0x63, 0xF8, 0xC0,
		0xF0, 0x87, 0xE1, 0x17, 0x83, 0x18, 0x43, 0x3E, 0x43, 0x38, 0x43, 0x39, 0x03, 0x3B, 0x00, 0x2C,
		0x98, 0x04, 0x41, 0xFA, 0xEC, 0xE7, 0x75, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,
		0x42, 0x60, 0x82,
	};

	CAutoFile file = ::CreateFile(testFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	ASSERT_TRUE(file.IsValid());
	DWORD written = 0;
	EXPECT_TRUE(::WriteFile(file, binaryFile, sizeof(binaryFile), &written, nullptr));
	EXPECT_EQ(sizeof(binaryFile), written);
	EXPECT_TRUE(file.CloseHandle());

	CString testFileSignature = testFile + L".rsa.asc";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFileSignature, L"-----BEGIN PGP SIGNATURE-----\nVersion: GnuPG v2\n\niQIcBAABCgAGBQJVxPvgAAoJEPfxez+d2VOerKsP/iQlt4S20LdUknT5udD8fFpT\nEFhZDPTXMRWkd8OhCLxa+t5b/ijyxFKWNz808Q7BDQcQoOQtC19NDvVmAcfqXflt\nR4Z+ugCG/lqP+3y0xBrL9m4UT9fNzQCiUY8Xpaih+6riLGnXVVpUe29BgVZppEWn\nMG9RVXpXwC+M3VQ0yQWu1F2Yy5OjdH4Ww5kpluoseZMOPhT6VaSfXeyQmg2DrBbw\nUfVV+w718noZ5znDH9MiD5y+sd4o7vqN2YFdg6FNvTjHyha39aFV8UCpPX1lH9ME\nN9v1vt5IWGCWL0zsZ6umHqibGG3Q2S3mlqktFhKJGWci+Swy4cNMpXDHIKY6e5v7\nxPIP92djyFtbjdixcSqBaYFC/Dd0KuCa1eYmyi2KxzUP29rZ+EHWoazfbeL1Y2Pu\nkSBrVFv64j0URzSMxUpJMqYXZRC5QW7vdbVwHGAPzoS+0rBBddwfSKteBQjagcHA\nkLk3sAIZNj1JaP5dcGL2K4Wxlaae0WgwI48lZSBMoL8SaInQvcKq9iL64xfpU+FU\nG0mzbidvTfZpyEU3eeSNiFi+6z4XLQ3NUFxsOr5jEPVKvgRoPljhJ4nCx034KQRQ\nHbVF9KYgMJSroKN9bNi/UFkC45Pc4wVov/XyH82XCVS30Du4hsVXsTdAiiXb3i6w\nkrWIJWYRxx76XGtQoHrR\n=PlFQ\n-----END PGP SIGNATURE-----"));
	EXPECT_EQ(0, VerifyIntegrity(testFile, testFileSignature, nullptr));

	// signature is shorter than pubkey; did not work w/o commit 941c103c00e48b0ffe592ab4d87d40ff48f899f6
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFileSignature, L"-----BEGIN PGP SIGNATURE-----\nVersion: GnuPG v2\n\niQIbBAABCgAGBQJVxQQOAAoJEPfxez+d2VOe7GoP+KflvYJVpNXSwnteXyDc1zTo\nJAyAJFtW6mgqCLrN6KoMrncEvd0cHrD07hT5mxULkbP6zy9fGaFYF0gHO0mOqErF\nIIbDcpPTWooJWUHMbnuQjf2I0x70Cr8ikycZQ7uDg7scZx7E8EIwHOJX60dcfdVX\nK1SQ+c+VIvo1uxL1PndloACaeINz6rL0Pj504lVDJaaGBvzdAV6KEIcjjm+Jb4r0\n6CYEntmt1i3Mc8FM6Xp1QfGtJqp4ogjv6o89hvtaBBLihB54EVQn66/2qIjdhX8W\nkdqJE6+pmFTfuD02XNECwl6stDIZcxXCw6EE87/1hdHip8oW9enIVTxKWB3fUWHl\n236eE2qb8zAN/WWGx/2Cvi6ctyosy9Cc/1hopnmV0KgZFfmDJWw0lISlC/3ZYWEe\nQFiXs5FPzLwfKnkq4REpoOKKfbZNk329J2iLFnwrF0mlL4vUZ1m26BBBk6lhm41J\nnjOSFWhJG/AqBbR5DqdJfXMqnxpfVZfaE5cry2rhjI4mfzw0xzrgvL4M9FZ7R9F8\njVHAV11CMqtp/7gMHXu6ljkTEBBQ7cgbU5DEAwfWddaJF+R30jmNSVliN2JW2FAk\nIbE8yV+uPiyXebn49CmRzkOWOAZqx+DShqriXGam37qjhdys147wPMMiwk8lZKjP\n4V66gTFw45UvBpophyg=\n=0bmW\n-----END PGP SIGNATURE-----"));
	EXPECT_EQ(0, VerifyIntegrity(testFile, testFileSignature, nullptr));
}

TEST(UpdateCrypto, BinarySignature)
{
	CAutoTempDir tempdir;
	CString testFile = tempdir.GetTempDir() + L"\\test.txt";

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"Text with\r\nNewline"));

	unsigned char binarySignature[543] =
	{
		0x89, 0x02, 0x1C, 0x04, 0x00, 0x01, 0x0A, 0x00, 0x06, 0x05, 0x02, 0x55, 0xC5, 0x03, 0x29, 0x00,
		0x0A, 0x09, 0x10, 0xF7, 0xF1, 0x7B, 0x3F, 0x9D, 0xD9, 0x53, 0x9E, 0x1E, 0xDF, 0x0F, 0xFF, 0x6C,
		0xE9, 0x7B, 0xE0, 0xD6, 0x97, 0x1A, 0xC0, 0x07, 0x77, 0x1C, 0xAB, 0x63, 0x67, 0x9A, 0x75, 0x34,
		0xF7, 0x17, 0x84, 0xE2, 0x52, 0xD8, 0x79, 0x4E, 0x35, 0x60, 0xF6, 0x2D, 0x30, 0x1D, 0xF1, 0x09,
		0x55, 0x56, 0x4D, 0x00, 0x2E, 0xBF, 0x2C, 0x1A, 0xCA, 0xDE, 0xEF, 0xA2, 0x86, 0xF5, 0xE1, 0x07,
		0x7B, 0x48, 0xB5, 0x42, 0x73, 0x59, 0x7D, 0xE0, 0xC9, 0x57, 0x86, 0x06, 0x12, 0xAE, 0xF0, 0xC4,
		0xF1, 0x73, 0x9B, 0x23, 0xB8, 0xF8, 0x67, 0x99, 0xB0, 0xB3, 0xCA, 0x0F, 0xF9, 0x1F, 0xD1, 0x5D,
		0xD3, 0xFD, 0x44, 0x5C, 0x30, 0x17, 0x4D, 0x84, 0x3A, 0x4E, 0x7B, 0x01, 0xB4, 0x07, 0x37, 0x70,
		0x0F, 0x5D, 0xE1, 0x8C, 0x1A, 0x6B, 0xAE, 0x7D, 0x5E, 0x58, 0x6F, 0x16, 0xE8, 0xBD, 0x3D, 0xC1,
		0x83, 0x89, 0x52, 0x56, 0x57, 0x5E, 0x3A, 0xA6, 0xD1, 0x5E, 0xC6, 0xB0, 0x3F, 0x8D, 0xDC, 0xA8,
		0xAF, 0x32, 0xC4, 0x30, 0x33, 0x9A, 0x5F, 0x5F, 0xF9, 0xAA, 0xFE, 0xCE, 0xBA, 0x4C, 0x86, 0xFD,
		0xC1, 0x46, 0x85, 0x0B, 0x0B, 0x5F, 0xF3, 0x78, 0x55, 0x74, 0x85, 0x13, 0x0D, 0xA0, 0x4B, 0xEF,
		0x36, 0x37, 0xB0, 0xC8, 0x8B, 0x16, 0x10, 0x2D, 0x97, 0xBD, 0x9E, 0x83, 0x14, 0x70, 0x76, 0xD5,
		0x3C, 0xC1, 0x70, 0x35, 0xEF, 0xD1, 0x0D, 0xDA, 0x0B, 0xEA, 0x2B, 0x3F, 0x31, 0x24, 0x26, 0xF8,
		0x6E, 0x2F, 0x43, 0x4B, 0xDD, 0xC2, 0x2F, 0x7B, 0x98, 0x3B, 0x1B, 0x6B, 0x3B, 0x12, 0x6D, 0x94,
		0xC2, 0x04, 0x41, 0x16, 0xF0, 0xB7, 0xE1, 0xBD, 0x2B, 0x35, 0x3F, 0x0D, 0x3A, 0x8B, 0x2D, 0xE0,
		0x1A, 0x07, 0x3E, 0x51, 0xB6, 0xD7, 0x06, 0x27, 0x2E, 0x71, 0x8F, 0x88, 0x0A, 0xC0, 0xF9, 0xE4,
		0x0D, 0x71, 0x86, 0x9F, 0x80, 0xCB, 0x72, 0xC4, 0x8F, 0x2E, 0x4E, 0x7B, 0x83, 0x35, 0xBA, 0xAD,
		0x73, 0xAB, 0x8A, 0x69, 0xEE, 0xCE, 0xA7, 0x34, 0x69, 0x88, 0xF7, 0x39, 0x13, 0xE9, 0x67, 0xE5,
		0x4E, 0x74, 0x16, 0xAB, 0xA4, 0x84, 0xD4, 0x7A, 0x19, 0x5A, 0x71, 0x72, 0xC9, 0xCD, 0x52, 0xFA,
		0xE8, 0x16, 0x33, 0xAF, 0x0B, 0x78, 0xC8, 0xA9, 0x05, 0x57, 0xD1, 0x6A, 0x0D, 0x46, 0x36, 0x46,
		0x45, 0x4A, 0xC4, 0x64, 0x81, 0xC8, 0x0E, 0x8A, 0xDC, 0xF3, 0x75, 0xD2, 0x5D, 0x60, 0x86, 0xE9,
		0x23, 0x02, 0xFA, 0x9C, 0xF5, 0x95, 0xEE, 0x80, 0x47, 0x7F, 0xD1, 0xEE, 0xCC, 0xCF, 0x2D, 0x62,
		0x3E, 0x8A, 0x29, 0xEE, 0x8E, 0x43, 0x5F, 0x74, 0xED, 0xB1, 0x8D, 0xC7, 0x41, 0x5A, 0x51, 0xC8,
		0xE6, 0xCE, 0xCE, 0x03, 0x77, 0x86, 0x60, 0xEA, 0x73, 0xFC, 0x6D, 0x62, 0x62, 0xAF, 0x6C, 0x2A,
		0x1F, 0x84, 0xC4, 0x10, 0x61, 0x78, 0x51, 0x38, 0x8B, 0x4F, 0xE3, 0xDA, 0x39, 0xB6, 0x32, 0x58,
		0x84, 0x0C, 0xE7, 0xCD, 0xDF, 0xB1, 0x6D, 0x72, 0xFD, 0xF6, 0x22, 0x50, 0xB0, 0x7F, 0x35, 0x56,
		0xF0, 0x9B, 0x47, 0xF1, 0x45, 0x13, 0x06, 0xE5, 0x71, 0x15, 0xE9, 0xB2, 0x5E, 0x78, 0xE0, 0x70,
		0xAB, 0x98, 0xBF, 0xF3, 0x15, 0x6A, 0x3D, 0x3E, 0xDF, 0x2D, 0x23, 0xBF, 0x16, 0x96, 0x0D, 0xF3,
		0x4F, 0xD4, 0x48, 0x66, 0x2E, 0x52, 0xB0, 0x48, 0x2E, 0x37, 0x7A, 0xCA, 0xFA, 0x89, 0x3B, 0x21,
		0xAA, 0xD1, 0x96, 0x1C, 0x2E, 0x28, 0x7A, 0xEF, 0xCB, 0x09, 0x62, 0xF4, 0xC5, 0x3E, 0x81, 0x87,
		0xBB, 0x9B, 0x48, 0xAB, 0x1E, 0xD5, 0xAE, 0x3B, 0x20, 0x1A, 0xD1, 0x51, 0x9D, 0xE0, 0x6E, 0x6E,
		0xCC, 0x29, 0x24, 0x34, 0xCF, 0x07, 0xF4, 0x33, 0x42, 0x3A, 0xF2, 0x86, 0x4F, 0xFA, 0xEE, 0x5D,
		0x0A, 0x6A, 0x33, 0xE0, 0xFC, 0xBB, 0x1C, 0xA0, 0xA4, 0x76, 0xE8, 0xB1, 0x17, 0xF5, 0xE4,
	};

	CString testFileSignature = testFile + L".rsa.asc";
	CAutoFile file = ::CreateFile(testFileSignature, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	ASSERT_TRUE(file.IsValid());
	DWORD written = 0;
	EXPECT_TRUE(::WriteFile(file, binarySignature, sizeof(binarySignature), &written, nullptr));
	EXPECT_EQ(sizeof(binarySignature), written);
	EXPECT_TRUE(file.CloseHandle());

	EXPECT_EQ(0, VerifyIntegrity(testFile, testFileSignature, nullptr));

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(testFile, L"Text with\nNewline"));
	EXPECT_EQ(-1, VerifyIntegrity(testFile, testFileSignature, nullptr));
}
