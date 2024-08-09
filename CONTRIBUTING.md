# Contributing To Cppcheck

These are some guidelines *any* contributor should follow. They will help to make for better contributions which will most likely allow for them to be processed.

(TODO: move daca@home section here?)

## Code Changes

Code contributions are handled via GitHub pull requests: https://github.com/danmar/cppcheck/pulls.

If you file a pull request you might not get a reply immediately. We are a very small team and it might not fit in the current scope or time.

Any kind of contribution is welcome but we might reject it. In that case we usually provide an explanation for the reasons but everything is always open to discussion.

Also after you filed a pull request please be ready to reply to questions and feedback. If you just "dump and leave" it might lowered the changes of your change being accepted.

Please be not discouraged if your change was rejected or if the review process might not have as smooth as it could have been.

Each change should be accompanied with a unit (C++) or integration (Python) test to ensure that it doesn't regress with future changes. Negative tests (testing the opposite behavior) would be favorable but might not be required or might already exist depending on the change.

The CI is already doing a lot of work but there are certain parts it cannot ensure.

The CI has an "always green" approach which means that failing tests are not allowed. Flakey tests might be acceptable depending on the frequency of their failures but they should be accompanied by ticket so they are being tracked. If you introducing a test which is expected to fail you should use the `TODO_*` macros (C++) or `@pytest.mark.xfail(strict=False)` annotations.

Note: Usually you can run the CI on your own fork to verify that is passes before even open an PR. Unfortunately some changes to avoid duplicated build in our CI disabled this.