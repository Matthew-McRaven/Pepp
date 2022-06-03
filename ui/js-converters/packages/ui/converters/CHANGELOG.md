# Change Log

All notable changes to this project will be documented in this file.
See [Conventional Commits](https://conventionalcommits.org) for commit guidelines.

## [0.4.1](https://gitlab.com/pep10/pepsuite/compare/v0.4.0...v0.4.1) (2022-06-03)

**Note:** Version bump only for package @pep10/ui-converters





# [0.4.0](https://gitlab.com/pep10/pepsuite/compare/v0.2.1...v0.4.0) (2022-05-27)


### Bug Fixes

* **ui:** accept empty strings in UnicodeConverter ([fb00806](https://gitlab.com/pep10/pepsuite/commit/fb0080632ef377eba991d0cc11ad322367fe78df))
* **ui:** don't reset on invallid character entry ([5cac8e6](https://gitlab.com/pep10/pepsuite/commit/5cac8e657e4c2f22007c4e5bdde241e796c772c3)), closes [#313](https://gitlab.com/pep10/pepsuite/issues/313) [#316](https://gitlab.com/pep10/pepsuite/issues/316)
* **ui:** fix broken MapConverter test  ([5490fe7](https://gitlab.com/pep10/pepsuite/commit/5490fe7149bb91e640803f2323db0bd2b6db62e3))
* **ui:** fix broken storybook tests ([d348212](https://gitlab.com/pep10/pepsuite/commit/d348212518a8f32c89d056767ec1be3420c93452)), closes [#349](https://gitlab.com/pep10/pepsuite/issues/349)
* **ui:** fix broken unit test for MapConverter ([074ec50](https://gitlab.com/pep10/pepsuite/commit/074ec50455b4c63551b7a8c283347cd3b8c7d453)), closes [#331](https://gitlab.com/pep10/pepsuite/issues/331)
* **ui:** fix MapConverter style regression on RO ([d749dc4](https://gitlab.com/pep10/pepsuite/commit/d749dc40998dea011f9348fe457d9b850e3e6f3a)), closes [#330](https://gitlab.com/pep10/pepsuite/issues/330)
* **ui:** fix unclearable signed IntegralConverter ([fef9bc0](https://gitlab.com/pep10/pepsuite/commit/fef9bc00b52e672058f19d9a7dfa08874fffc8b3)), closes [#322](https://gitlab.com/pep10/pepsuite/issues/322)
* **ui:** improve `-` handling for signed decimals ([1f87525](https://gitlab.com/pep10/pepsuite/commit/1f87525c04bb62af1b3805d9d1d48a9261214bd7)), closes [#353](https://gitlab.com/pep10/pepsuite/issues/353) [#321](https://gitlab.com/pep10/pepsuite/issues/321)
* **ui:** limit length of UnicodeConverter input ([b0cc762](https://gitlab.com/pep10/pepsuite/commit/b0cc762e13307da8a95f7e1c8e7dc4d17593752b)), closes [#323](https://gitlab.com/pep10/pepsuite/issues/323)
* **ui:** must strip base prefix in IntegralConverter ([772775e](https://gitlab.com/pep10/pepsuite/commit/772775e846d2d78bfcc766cc55dfce1ac7c818c5))
* **ui:** prevent text movement on switch to RO ([9cfaaa2](https://gitlab.com/pep10/pepsuite/commit/9cfaaa249264e09d3215f93b0261d9b03a8c33fd)), closes [#330](https://gitlab.com/pep10/pepsuite/issues/330)
* **ui:** stop clearing SignedDecimal on invalid input ([2b2566a](https://gitlab.com/pep10/pepsuite/commit/2b2566a5aebc635c2d956e257ecb1cd342521757)), closes [#368](https://gitlab.com/pep10/pepsuite/issues/368)
* **ui:** stop using BigInt literals ([0f90e2c](https://gitlab.com/pep10/pepsuite/commit/0f90e2c773ae82fe8b31cf1a5d63dbb3562b3094))


### Features

* **ui:** `-` toggles sign on SignedDecimalConverter ([53015ea](https://gitlab.com/pep10/pepsuite/commit/53015eae2977653efe5143c2501a585ce0189804)), closes [#321](https://gitlab.com/pep10/pepsuite/issues/321)
* **ui:** allow converter to have no prefix ([f733ec8](https://gitlab.com/pep10/pepsuite/commit/f733ec8e40a88feab6071ac6faab9dd0d45c3670)), closes [#350](https://gitlab.com/pep10/pepsuite/issues/350)
* **ui:** explicitly enumerate supported bases ([be6685c](https://gitlab.com/pep10/pepsuite/commit/be6685c5ea71d9dca16085907d2a1559d987c8b3))





# [0.3.0](https://gitlab.com/pep10/pepsuite/compare/v0.2.1...v0.3.0) (2022-05-14)


### Bug Fixes

* **ui:** accept empty strings in UnicodeConverter ([fb00806](https://gitlab.com/pep10/pepsuite/commit/fb0080632ef377eba991d0cc11ad322367fe78df))
* **ui:** don't reset on invallid character entry ([5cac8e6](https://gitlab.com/pep10/pepsuite/commit/5cac8e657e4c2f22007c4e5bdde241e796c772c3)), closes [#313](https://gitlab.com/pep10/pepsuite/issues/313) [#316](https://gitlab.com/pep10/pepsuite/issues/316)
* **ui:** fix broken MapConverter test  ([5490fe7](https://gitlab.com/pep10/pepsuite/commit/5490fe7149bb91e640803f2323db0bd2b6db62e3))
* **ui:** fix broken storybook tests ([d348212](https://gitlab.com/pep10/pepsuite/commit/d348212518a8f32c89d056767ec1be3420c93452)), closes [#349](https://gitlab.com/pep10/pepsuite/issues/349)
* **ui:** fix broken unit test for MapConverter ([074ec50](https://gitlab.com/pep10/pepsuite/commit/074ec50455b4c63551b7a8c283347cd3b8c7d453)), closes [#331](https://gitlab.com/pep10/pepsuite/issues/331)
* **ui:** fix MapConverter style regression on RO ([d749dc4](https://gitlab.com/pep10/pepsuite/commit/d749dc40998dea011f9348fe457d9b850e3e6f3a)), closes [#330](https://gitlab.com/pep10/pepsuite/issues/330)
* **ui:** fix unclearable signed IntegralConverter ([fef9bc0](https://gitlab.com/pep10/pepsuite/commit/fef9bc00b52e672058f19d9a7dfa08874fffc8b3)), closes [#322](https://gitlab.com/pep10/pepsuite/issues/322)
* **ui:** improve `-` handling for signed decimals ([1f87525](https://gitlab.com/pep10/pepsuite/commit/1f87525c04bb62af1b3805d9d1d48a9261214bd7)), closes [#353](https://gitlab.com/pep10/pepsuite/issues/353) [#321](https://gitlab.com/pep10/pepsuite/issues/321)
* **ui:** limit length of UnicodeConverter input ([b0cc762](https://gitlab.com/pep10/pepsuite/commit/b0cc762e13307da8a95f7e1c8e7dc4d17593752b)), closes [#323](https://gitlab.com/pep10/pepsuite/issues/323)
* **ui:** must strip base prefix in IntegralConverter ([772775e](https://gitlab.com/pep10/pepsuite/commit/772775e846d2d78bfcc766cc55dfce1ac7c818c5))
* **ui:** prevent text movement on switch to RO ([9cfaaa2](https://gitlab.com/pep10/pepsuite/commit/9cfaaa249264e09d3215f93b0261d9b03a8c33fd)), closes [#330](https://gitlab.com/pep10/pepsuite/issues/330)
* **ui:** stop clearing SignedDecimal on invalid input ([2b2566a](https://gitlab.com/pep10/pepsuite/commit/2b2566a5aebc635c2d956e257ecb1cd342521757)), closes [#368](https://gitlab.com/pep10/pepsuite/issues/368)
* **ui:** stop using BigInt literals ([0f90e2c](https://gitlab.com/pep10/pepsuite/commit/0f90e2c773ae82fe8b31cf1a5d63dbb3562b3094))


### Features

* **ui:** `-` toggles sign on SignedDecimalConverter ([53015ea](https://gitlab.com/pep10/pepsuite/commit/53015eae2977653efe5143c2501a585ce0189804)), closes [#321](https://gitlab.com/pep10/pepsuite/issues/321)
* **ui:** allow converter to have no prefix ([f733ec8](https://gitlab.com/pep10/pepsuite/commit/f733ec8e40a88feab6071ac6faab9dd0d45c3670)), closes [#350](https://gitlab.com/pep10/pepsuite/issues/350)
* **ui:** explicitly enumerate supported bases ([be6685c](https://gitlab.com/pep10/pepsuite/commit/be6685c5ea71d9dca16085907d2a1559d987c8b3))





## [0.2.1](https://gitlab.com/pep10/pepsuite/compare/v0.2.0...v0.2.1) (2022-01-20)

**Note:** Version bump only for package @pep10/ui-converters





# 0.2.0 (2022-01-20)


### Bug Fixes

* **ui:** fix default case for parser ([6b20730](https://gitlab.com/pep10/pepsuite/commit/6b207300ca8013706cb54ffd0625985904ab6da5)), closes [#296](https://gitlab.com/pep10/pepsuite/issues/296)
