# Move the team project to GitLab

Use the professor's designated GitLab instance and course group. If a team project already exists, coordinate with Sydney/Henry and use it rather than creating a duplicate. Confirm whether GitLab replaces only repository hosting or also Jira/Confluence before moving course records.

## Recommended migration

1. A repository maintainer imports the existing GitHub repository using GitLab's GitHub importer, if available. This can preserve code/history plus supported pull-request and issue records. Import availability depends on the instance configuration.
2. If the importer is unavailable, create an empty GitLab project (no README initialization) and push the existing Git history. This transfers code/commits, not Jira issues, Confluence documents or GitHub PR discussions. Preserve those records separately.
3. Invite all nine members and the instructor/TA as required. Developers need appropriate push access; Sydney/Henry should manage repository settings. Review roles and protect main with merge-request review as agreed by the team.
4. Verify the imported main includes Sydney's merged architecture work. Have each member verify their branches and commit attribution. Keep GitHub as a reference until migration is verified; agree which remote is authoritative for future development.

## Add the new remote locally

Replace the placeholder with the clone URL copied from your actual GitLab project:

```sh
git remote add gitlab <GITLAB_CLONE_URL>
git remote -v
```

For an empty destination only, a maintainer can publish existing main:

```sh
git push gitlab main
```

If the GitLab project already contains commits, fetch and inspect its history before pushing. Do not force-push over teammates' work. A normal push preserves the ancestry of the pushed branch; other branches/tags must be imported or published separately.

After reviewing and committing Arvin's local changes:

```sh
git push -u gitlab feature/arvin-engine-infrastructure
```

Create a merge request into main. Adding/pushing a remote does not publish uncommitted files. The current origin remains GitHub until deliberately changed; using a separate `gitlab` remote avoids losing its address.

## Work tracking and CI

If GitLab is now the task tracker, create an infrastructure issue with acceptance criteria and an estimate, put it on the team's board/milestone according to course sprint expectations, and link it from your merge request. Record earlier work honestly; do not backdate issues. Keep individual development records in the location the professor requires.

The current `.github/workflows/ci.yml` is GitHub Actions configuration. It does not run as GitLab CI. Once the course runner environment is known, add `.gitlab-ci.yml` to run CMake build and CTest with an appropriate compiler image/runner. No GitLab pipeline has been configured or validated in this branch.

Official references: [GitHub import](https://docs.gitlab.com/user/project/import/github/), [repository migration](https://docs.gitlab.com/user/import/third_party_systems/repo_by_url/), [project members](https://docs.gitlab.com/user/project/members/).
