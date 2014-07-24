@wip
Feature: Inbox content
  As someone collecting tasks and notes
  I can display the Inbox
  In order to see the artifacts which need to be organized (e.g. any task or note not associated to any project, context or topic)

  Scenario: Unorganized tasks and notes appear in the inbox
   Given I'm looking at the inbox view
   When I look at the central list
   Then the list is:
      | display          |
      | Buy cheese       |
      | Buy apples       |
      | Buy pears        |
      | 21/04/2014 14:49 |
