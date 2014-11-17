Feature: Context removal
  As someone using tasks
  I can remove a context
  In order to maintain their semantic

@wip
  Scenario: Removed context disappear from the list
    Given I display the available pages
    When I remove a "context" named "Online"
    And I list the items
    Then the list is:
       | display                           | icon                |
       | Contexts                          | folder              |
       | Contexts / Chores                 | view-pim-tasks      |
       | Contexts / Internet               | view-pim-tasks      |
       | Inbox                             | mail-folder-inbox   |
       | Projects                          | folder              |
       | Projects / Backlog                | view-pim-tasks      |
       | Projects / Prepare talk about TDD | view-pim-tasks      |
       | Projects / Read List              | view-pim-tasks      |
       | Tags                              | folder              |
       | Tags / Philosophy                 | view-pim-tasks      |
       | Tags / Physics                    | view-pim-tasks      |

