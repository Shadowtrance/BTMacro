#pragma once

class MyShortcut
{
  private:
    int shortcutId;

  public:
    MyShortcut(int caseId);
    void Action();
    void ReleaseAllKeys();
};
