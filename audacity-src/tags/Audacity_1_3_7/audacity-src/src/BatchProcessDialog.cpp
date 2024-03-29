/**********************************************************************

  Audacity: A Digital Audio Editor

  BatchProcessDialog.cpp

  Dominic Mazzoni
  James Crook

*******************************************************************//*!

\class BatchProcessDialog
\brief Shows progress in executing commands in BatchCommands.

*//*******************************************************************/

#include "Audacity.h"

#include <wx/defs.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/intl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/listctrl.h>
#include <wx/radiobut.h>
#include <wx/button.h>
#include <wx/imaglist.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>

#include "Prefs.h"
#include "Project.h"
#include "BatchProcessDialog.h"
#include "Internat.h"
#include "commands/CommandManager.h"
#include "effects/Effect.h"
#include "../images/Arrow.xpm"
#include "BatchCommands.h"

#include "Theme.h"
#include "AllThemeResources.h"

#include "FileDialog.h"

#define ChainsListID       7001
#define ApplyToProjectID   7002
#define ApplyToFilesID     7003

BEGIN_EVENT_TABLE(BatchProcessDialog, wxDialog)
   EVT_BUTTON(ApplyToProjectID, BatchProcessDialog::OnApplyToProject)
   EVT_BUTTON(ApplyToFilesID, BatchProcessDialog::OnApplyToFiles)
   EVT_BUTTON(wxID_CANCEL, BatchProcessDialog::OnCancel)
END_EVENT_TABLE()

BatchProcessDialog::BatchProcessDialog(wxWindow * parent):
   wxDialog(parent, wxID_ANY, _("Apply Chain"),
            wxDefaultPosition, wxDefaultSize,
            wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
   AudacityProject * p = GetActiveProject();
   if (p->GetCleanSpeechMode())
   {
      SetTitle(_("CleanSpeech Batch Processing"));
   }

   SetLabel(_("Apply Chain"));         // Provide visual label
   SetName(_("Apply Chain"));          // Provide audible label
   Populate();

   mAbort = false;
}

BatchProcessDialog::~BatchProcessDialog()
{
}

void BatchProcessDialog::Populate()
{
   //------------------------- Main section --------------------
   ShuttleGui S(this, eIsCreating);
   PopulateOrExchange(S);
   // ----------------------- End of main section --------------
}

/// Defines the dialog and does data exchange with it.
void BatchProcessDialog::PopulateOrExchange(ShuttleGui &S)
{
   S.StartVerticalLay(true);
   {
      S.StartStatic(_("&Select chain"), true);
      {
         S.SetStyle(wxSUNKEN_BORDER | wxLC_REPORT | wxLC_HRULES | wxLC_VRULES |
                     wxLC_SINGLE_SEL);
         mChains = S.Id(ChainsListID).AddListControlReportMode();
         mChains->InsertColumn(0, _("Chain"), wxLIST_FORMAT_LEFT);
      }
      S.EndStatic();

      S.StartHorizontalLay(wxALIGN_RIGHT, false);
      {
         S.SetBorder(10);
         S.Id(ApplyToProjectID).AddButton(_("Apply to Current &Project"));
         S.Id(ApplyToFilesID).AddButton(_("Apply to &Files..."));
         S.Id(wxID_CANCEL).AddButton(_("&Cancel"));
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();

   wxArrayString names = mBatchCommands.GetNames();
   for (int i = 0; i < (int)names.GetCount(); i++) {
      mChains->InsertItem(i, names[i]);
   }

   // Get and validate the currently active chain
   wxString name = gPrefs->Read(wxT("/Batch/ActiveChain"), wxT("CleanSpeech"));
   int item = mChains->FindItem(-1, name);
   if (item == -1) {
      item = 0;
      name = mChains->GetItemText(0);
   }

   // Select the name in the list...this will fire an event.
   mChains->SetItemState(item, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

   Layout();
   Fit();
   SetSizeHints(GetSize());
   Center();

   // Set the column size for the chains list.
   wxSize sz = mChains->GetClientSize();
   mChains->SetColumnWidth(0, sz.x);
}

void BatchProcessDialog::OnApplyToProject(wxCommandEvent &event)
{
   long item = mChains->GetNextItem(-1,
                                    wxLIST_NEXT_ALL,
                                    wxLIST_STATE_SELECTED);
   if (item == -1) {
      wxMessageBox(_("No chain selected"));
      return;
   }
   wxString name = mChains->GetItemText(item);

   wxDialog d(this, wxID_ANY, GetTitle());
   ShuttleGui S(&d, eIsCreating);

   S.StartHorizontalLay(wxCENTER, false);
   {
      S.StartStatic(wxT(""), false);   // deliberately not translated (!)
      {
         S.SetBorder(20);
         S.AddFixedText(wxString::Format(_("Applying '%s' to current project"),
                                         name.c_str()));
      }
      S.EndStatic();
   }
   S.EndHorizontalLay();

   d.Layout();
   d.Fit();
   d.CenterOnScreen();
   d.Move(-1, 0);
   d.Show();
   Hide();

   wxWindowDisabler wd;

   gPrefs->Write(wxT("/Batch/ActiveChain"), name);

   mBatchCommands.ReadChain(name);
   if (!mBatchCommands.ApplyChain()) {
      return;
   }

   EndModal(true);
}

void BatchProcessDialog::OnApplyToFiles(wxCommandEvent &event)
{
   long item = mChains->GetNextItem(-1,
                                    wxLIST_NEXT_ALL,
                                    wxLIST_STATE_SELECTED);
   if (item == -1) {
      wxMessageBox(_("No chain selected"));
      return;
   }

   wxString name = mChains->GetItemText(item);
   gPrefs->Write(wxT("/Batch/ActiveChain"), name);

   AudacityProject *project = GetActiveProject();
   if (!project->GetIsEmpty()) {
      wxMessageBox(_("Please save and close the current project first."));
      return;
   }

   wxString path = gPrefs->Read(wxT("/DefaultOpenPath"), ::wxGetCwd());
   wxString prompt =  project->GetCleanSpeechMode() ? 
      _("Select vocal file(s) for batch CleanSpeech Chain...") :
      _("Select file(s) for batch processing...");
   wxString fileSelector = project->GetCleanSpeechMode() ? 
      _("Vocal files (*.wav;*.mp3)|*.wav;*.mp3|WAV files (*.wav)|*.wav|MP3 files (*.mp3)|*.mp3") :
      _("All files (*.*)|*.*|WAV files (*.wav)|*.wav|AIFF files (*.aif)|*.aif|AU files (*.au)|*.au|MP3 files (*.mp3)|*.mp3|Ogg Vorbis files (*.ogg)|*.ogg|FLAC files (*.flac)|*.flac"
       );

   FileDialog dlog(this, prompt,
                   path, wxT(""), fileSelector,
                   wxFD_OPEN | wxFD_MULTIPLE | wxRESIZE_BORDER);

   if (dlog.ShowModal() != wxID_OK) {
      return;
   }

   wxArrayString files;
   dlog.GetPaths(files);

   files.Sort();

   wxDialog d(this, wxID_ANY, GetTitle());
   ShuttleGui S(&d, eIsCreating);

   S.StartVerticalLay(false);
   {
      S.StartStatic(_("Applying..."), 1);
      {
         wxImageList *imageList = new wxImageList(9, 16);
         imageList->Add(wxIcon(empty_9x16_xpm));
         imageList->Add(wxIcon(arrow_xpm));

         S.SetStyle(wxSUNKEN_BORDER | wxLC_REPORT | wxLC_HRULES | wxLC_VRULES |
                    wxLC_SINGLE_SEL);
         mList = S.AddListControlReportMode();
         mList->AssignImageList(imageList, wxIMAGE_LIST_SMALL);
         mList->InsertColumn(0, _("File"), wxLIST_FORMAT_LEFT);
      }
      S.EndStatic();

      S.StartHorizontalLay(wxCENTER, false);
      {
         S.Id(wxID_CANCEL).AddButton(_("&Cancel"));
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();

   int i;
   for (i = 0; i < (int)files.GetCount(); i++ ) {
      mList->InsertItem(i, files[i], i == 0);
   }

   // Set the column size for the files list.
   mList->SetColumnWidth(0, wxLIST_AUTOSIZE);

   int width = mList->GetColumnWidth(0);
   wxSize sz = mList->GetClientSize();
   if (width > sz.GetWidth() && width < 500) {
      sz.SetWidth(width);
      mList->SetInitialSize(sz);
   }

   d.Layout();
   d.Fit();
   d.SetSizeHints(d.GetSize());
   d.CenterOnScreen();
   d.Move(-1, 0);
   d.Show();
   Hide();

   mBatchCommands.ReadChain(name);
   for (i = 0; i < (int)files.GetCount(); i++) {
      wxWindowDisabler wd(&d);
      if (i > 0) {
         //Clear the arrow in previous item.
         mList->SetItemImage(i - 1, 0, 0);
      }
      mList->SetItemImage(i, 1, 1);
      mList->EnsureVisible(i);

      project->OnRemoveTracks();
      project->Import(files[i]);
      project->OnSelectAll();
      if (!mBatchCommands.ApplyChain()) {
         break;
      }

      if (!d.IsShown() || mAbort) {
         break;
      }
   }
   project->OnRemoveTracks();

   EndModal(true);
}

void BatchProcessDialog::OnCancel(wxCommandEvent &event)
{
   EndModal(false);
}

/////////////////////////////////////////////////////////////////////
#include <wx/textdlg.h>
#include "../BatchCommandDialog.h"

enum {
// ChainsListID             7005
   AddButtonID = 10000,
   RemoveButtonID,
   CommandsListID,
   ImportButtonID,
   ExportButtonID,
   DefaultsButtonID,
   InsertButtonID,
   DeleteButtonID,
   UpButtonID,
   DownButtonID,
   RenameButtonID
};

BEGIN_EVENT_TABLE(EditChainsDialog, wxDialog)
   EVT_LIST_ITEM_SELECTED(ChainsListID, EditChainsDialog::OnChainSelected)
   EVT_LIST_BEGIN_LABEL_EDIT(ChainsListID, EditChainsDialog::OnChainsBeginEdit)
   EVT_LIST_END_LABEL_EDIT(ChainsListID, EditChainsDialog::OnChainsEndEdit)
   EVT_BUTTON(AddButtonID, EditChainsDialog::OnAdd)
   EVT_BUTTON(RemoveButtonID, EditChainsDialog::OnRemove)
   EVT_BUTTON(RenameButtonID, EditChainsDialog::OnRename)

   EVT_LIST_ITEM_ACTIVATED(CommandsListID, EditChainsDialog::OnCommandActivated)
   EVT_BUTTON(InsertButtonID, EditChainsDialog::OnInsert)
   EVT_BUTTON(DeleteButtonID, EditChainsDialog::OnDelete)
   EVT_BUTTON(UpButtonID, EditChainsDialog::OnUp)
   EVT_BUTTON(DownButtonID, EditChainsDialog::OnDown)
   EVT_BUTTON(DefaultsButtonID, EditChainsDialog::OnDefaults)

   EVT_BUTTON(wxID_OK, EditChainsDialog::OnOK)
   EVT_BUTTON(wxID_CANCEL, EditChainsDialog::OnCancel)

   EVT_KEY_DOWN(EditChainsDialog::OnKeyDown)
END_EVENT_TABLE()

enum {
   BlankColumn,   
   ItemNumberColumn,    
   ActionColumn, 
   ParamsColumn,
};

/// Constructor
EditChainsDialog::EditChainsDialog(wxWindow * parent):
   wxDialog(parent, wxID_ANY, _("Edit Chains"),
            wxDefaultPosition, wxDefaultSize,
            wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
   SetLabel(_("Edit Chains"));         // Provide visual label
   SetName(_("Edit Chains"));          // Provide audible label

   mChanged = false;
   mSelectedCommand = 0;

   Populate();
}

EditChainsDialog::~EditChainsDialog()
{
}

/// Creates the dialog and its contents.
void EditChainsDialog::Populate()
{
   //------------------------- Main section --------------------
   ShuttleGui S(this, eIsCreating);
   PopulateOrExchange(S);
   // ----------------------- End of main section --------------

   // Get and validate the currently active chain
   mActiveChain = gPrefs->Read(wxT("/Batch/ActiveChain"), wxT("CleanSpeech"));

   // Go populate the chains list.
   PopulateChains();

   // We have a bare list.  We need to add columns and content.
   PopulateList();

   // Layout and set minimum size of window
   Layout();
   Fit();
   SetSizeHints(GetSize());

   // Size and place window
   SetSize(wxSystemSettings::GetMetric(wxSYS_SCREEN_X) * 3 / 4,
           wxSystemSettings::GetMetric(wxSYS_SCREEN_Y) * 4 / 5);
   Center();

   // Set the column size for the chains list.
   wxSize sz = mChains->GetClientSize();
   mChains->SetColumnWidth(0, sz.x);

   // Size columns properly
   mList->SetColumnWidth(BlankColumn, 0); // First column width is zero, to hide it.
   mList->SetColumnWidth(ItemNumberColumn,  wxLIST_AUTOSIZE);
   mList->SetColumnWidth(ActionColumn, wxLIST_AUTOSIZE);
   mList->SetColumnWidth(ParamsColumn, wxLIST_AUTOSIZE);
}

/// Defines the dialog and does data exchange with it.
void EditChainsDialog::PopulateOrExchange(ShuttleGui & S)
{
   S.StartHorizontalLay(wxEXPAND, 1);
   {
      S.StartStatic(_("&Chains"));
      {
         // JKC: Experimenting with an alternative way to get multiline
         // translated strings to work correctly without very long lines.
         // My appologies Alexandre if this way didn't work either.
         // 
         // With this method:
         //   1) it compiles fine under windows unicode and normal mode.
         //   2) xgettext source code has handling for the trailing '\'
         //
         // It remains to see if linux and mac can cope and if xgettext 
         // actually does do fine with strings presented like this.
         // If it doesn't work out, revert to all-on-one-line.
         S.SetStyle(wxSUNKEN_BORDER | wxLC_REPORT | wxLC_HRULES | wxLC_SINGLE_SEL |
                    wxLC_EDIT_LABELS);
         mChains = S.Id(ChainsListID).AddListControlReportMode();
         mChains->InsertColumn(0, wxT("Chain"), wxLIST_FORMAT_LEFT);
         S.StartHorizontalLay(wxCENTER, false);
         {
            S.Id(AddButtonID).AddButton(_("&Add"));
            mRemove = S.Id(RemoveButtonID).AddButton(_("&Remove"));
            mRename = S.Id(RenameButtonID).AddButton(_("Re&name"));
         }
         S.EndHorizontalLay();
      }
      S.EndStatic();

      S.StartStatic(_("C&hain (Double-Click or press SPACE to edit)"), true);
      {
         S.SetStyle(wxSUNKEN_BORDER | wxLC_REPORT | wxLC_HRULES | wxLC_VRULES |
                    wxLC_SINGLE_SEL);
         mList = S.Id(CommandsListID).AddListControlReportMode();

         //An empty first column is a workaround - under Win98 the first column 
         //can't be right aligned.
         mList->InsertColumn(BlankColumn, wxT(""), wxLIST_FORMAT_LEFT);
         /* i18n-hint: This is the number of the command in the list */
         mList->InsertColumn(ItemNumberColumn, _("No"), wxLIST_FORMAT_RIGHT);
         mList->InsertColumn(ActionColumn, _NoAcc("&Command"), wxLIST_FORMAT_RIGHT);
         mList->InsertColumn(ParamsColumn, _NoAcc("&Parameters"), wxLIST_FORMAT_LEFT);

         S.StartHorizontalLay(wxCENTER, false);
         {
            S.Id(InsertButtonID).AddButton(_("&Insert"), wxALIGN_LEFT);
            S.Id(DeleteButtonID).AddButton(_("De&lete"), wxALIGN_LEFT);
            S.Id(UpButtonID).AddButton(_("Move &Up"), wxALIGN_LEFT);
            S.Id(DownButtonID).AddButton(_("Move &Down"), wxALIGN_LEFT);
            mDefaults = S.Id(DefaultsButtonID).AddButton(_("De&faults"));
         }
         S.EndHorizontalLay();
      }
      S.EndStatic();
   }
   S.EndHorizontalLay();

   S.AddStandardButtons();

   return;
}

/// This clears and updates the contents of mChains
void EditChainsDialog::PopulateChains()
{
   wxArrayString names = mBatchCommands.GetNames();
   int i;

   mChains->DeleteAllItems();
   for (i = 0; i < (int)names.GetCount(); i++) {
      mChains->InsertItem(i, names[i]);
   }

   int item = mChains->FindItem(-1, mActiveChain);
   if (item == -1) {
      item = 0;
      mActiveChain = mChains->GetItemText(0);
   }

   // Select the name in the list...this will fire an event.
   mChains->SetItemState(item, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
}

/// This clears and updates the contents of mList
void EditChainsDialog::PopulateList()
{
   mList->DeleteAllItems();

   for (int i = 0; i < mBatchCommands.GetCount(); i++) {
      AddItem(mBatchCommands.GetCommand(i),
              mBatchCommands.GetParams(i));
   }

   AddItem(_("- END -"), wxT(""));

   // Select the name in the list...this will fire an event.
   if (mSelectedCommand >= (int)mList->GetItemCount()) {
      mSelectedCommand = 0;
   }
   mList->SetItemState(mSelectedCommand, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

   // Size columns properly
   mList->SetColumnWidth(BlankColumn, 0); // First column width is zero, to hide it.
   mList->SetColumnWidth(ItemNumberColumn,  wxLIST_AUTOSIZE);
   mList->SetColumnWidth(ActionColumn, wxLIST_AUTOSIZE);
   mList->SetColumnWidth(ParamsColumn, wxLIST_AUTOSIZE);
}

/// Add one item into mList
void EditChainsDialog::AddItem(const wxString &Action, const wxString &Params)
{
   int i = mList->GetItemCount();

   mList->InsertItem(i, wxT(""));
   mList->SetItem(i, ItemNumberColumn, wxString::Format(wxT(" %02i"), i + 1));
   mList->SetItem(i, ActionColumn, Action );
   mList->SetItem(i, ParamsColumn, Params );
}

bool EditChainsDialog::ChangeOK()
{
   if (mChanged) {
      wxString title;
      wxString msg;
      int id;

      title.Printf(_("%s changed"), mActiveChain.c_str());
      msg = _("Do you want to save the changes?");

      id = wxMessageBox(msg, title, wxYES_NO | wxCANCEL);
      if (id == wxCANCEL) {
         return false;
      }

      if (id == wxYES) {
         if (!mBatchCommands.WriteChain(mActiveChain)) {
            return false;
         }
      }

      mChanged = false;
   }

   return true;
}
/// An item in the chains list has been selected.
void EditChainsDialog::OnChainSelected(wxListEvent &event)
{
   if (!ChangeOK()) {
      event.Veto();
      return;
   }

   int item = event.GetIndex();

   mActiveChain = mChains->GetItemText(item);
   mBatchCommands.ReadChain(mActiveChain);

   if (mBatchCommands.IsFixed(mActiveChain)) {
      mRemove->Disable();
      mRename->Disable();
      mDefaults->Enable();
   }
   else {
      mRemove->Enable();
      mRename->Enable();
      mDefaults->Disable();
   }

   PopulateList();

   mList->SetColumnWidth(ItemNumberColumn,  wxLIST_AUTOSIZE);
   mList->SetColumnWidth(ActionColumn, wxLIST_AUTOSIZE);
}

///
void EditChainsDialog::OnChainsBeginEdit(wxListEvent &event)
{
   int itemNo = event.GetIndex();

   wxString chain = mChains->GetItemText(itemNo);

   if (mBatchCommands.IsFixed(mActiveChain)) {
      wxBell();
      event.Veto();
   }
}

///
void EditChainsDialog::OnChainsEndEdit(wxListEvent &event)
{
   if (event.IsEditCancelled()) {
      return;
   }

   wxString newname = event.GetLabel();

   mBatchCommands.RenameChain(mActiveChain, newname);

   mActiveChain = newname;

   PopulateChains();
}

/// 
void EditChainsDialog::OnAdd(wxCommandEvent &event)
{
   while (true) {
      wxTextEntryDialog d(this,
                          _("Enter name of new chain"),
                          GetTitle());
      wxString name;

      if (d.ShowModal() == wxID_CANCEL) {
         return;
      }

      name = d.GetValue().Strip(wxString::both);

      if (name.Length() == 0) {
         wxMessageBox(_("Name must not be blank"),
                      GetTitle(),
                      wxOK | wxICON_ERROR,
                      this);
         continue;
      }

      if (name.Contains(wxFILE_SEP_PATH) ||
          name.Contains(wxFILE_SEP_PATH_UNIX)) {
         wxMessageBox(wxString::Format(_("Names may not contain '%c' and '%c'"),
                      wxFILE_SEP_PATH, wxFILE_SEP_PATH_UNIX),
                      GetTitle(),
                      wxOK | wxICON_ERROR,
                      this);
         continue;
      }

      mBatchCommands.AddChain(name);

      mActiveChain = name;

      PopulateChains();

      break;
   }
}

///
void EditChainsDialog::OnRemove(wxCommandEvent &event)
{
   long item = mChains->GetNextItem(-1,
                                    wxLIST_NEXT_ALL,
                                    wxLIST_STATE_SELECTED);
   if (item == -1) {
      return;
   }

   wxString name = mChains->GetItemText(item);
   wxMessageDialog m(this,
                     wxString::Format(_("Are you sure you want to delete %s?"), name.c_str()),
                     GetTitle(),
                     wxYES_NO | wxICON_QUESTION);
   if (m.ShowModal() == wxNO) {
      return;
   }

   mBatchCommands.DeleteChain(name);

   if (item >= (mChains->GetItemCount() - 1) && item >= 0) {
      item--;
   }

   mActiveChain = mChains->GetItemText(item);

   PopulateChains();
}

///
void EditChainsDialog::OnRename(wxCommandEvent &event)
{
   long item = mChains->GetNextItem(-1,
                                    wxLIST_NEXT_ALL,
                                    wxLIST_STATE_SELECTED);
   if (item == -1) {
      return;
   }

   mChains->EditLabel(item);
}

/// An item in the list has been selected.
/// Bring up a dialog to allow its parameters to be edited.
void EditChainsDialog::OnCommandActivated(wxListEvent &event)
{
   int item = event.GetIndex();

   BatchCommandDialog d(this, wxID_ANY);
   d.SetCommandAndParams(mBatchCommands.GetCommand(item),
                         mBatchCommands.GetParams(item));

   if (!d.ShowModal()) {
      return;
   }

   mBatchCommands.DeleteFromChain(item);
   mBatchCommands.AddToChain(d.mSelectedCommand,
                             d.mSelectedParameters,
                             item);

   mChanged = true;

   mSelectedCommand = item;

   PopulateList();
}

///
void EditChainsDialog::OnInsert(wxCommandEvent &event)
{
   long item = mList->GetNextItem(-1,
                                  wxLIST_NEXT_ALL,
                                  wxLIST_STATE_SELECTED);
   if (item == -1) {
      return;
   }

   BatchCommandDialog d(this, wxID_ANY);

   if (!d.ShowModal()) {
      return;
   }

   mBatchCommands.AddToChain(d.mSelectedCommand,
                             d.mSelectedParameters,
                             item);
   mChanged = true;

   mSelectedCommand = item + 1;

   PopulateList();
}

///
void EditChainsDialog::OnDelete(wxCommandEvent &event)
{
   long item = mList->GetNextItem(-1,
                                  wxLIST_NEXT_ALL,
                                  wxLIST_STATE_SELECTED);
   if (item == -1 || item + 1 == mList->GetItemCount()) {
      return;
   }

   mBatchCommands.DeleteFromChain(item);

   mChanged = true;

   if (item >= (mList->GetItemCount() - 2) && item >= 0) {
      item--;
   }

   mSelectedCommand = item;

   PopulateList();
}

///
void EditChainsDialog::OnUp(wxCommandEvent &event)
{
   long item = mList->GetNextItem(-1,
                                  wxLIST_NEXT_ALL,
                                  wxLIST_STATE_SELECTED);
   if (item == -1 || item == 0 || item + 1 == mList->GetItemCount()) {
      return;
   }

   mBatchCommands.AddToChain(mBatchCommands.GetCommand(item),
                             mBatchCommands.GetParams(item),
                             item - 1);
   mBatchCommands.DeleteFromChain(item + 1);

   mChanged = true;

   mSelectedCommand = item - 1;

   PopulateList();
}

///
void EditChainsDialog::OnDown(wxCommandEvent &event)
{
   long item = mList->GetNextItem(-1,
                                  wxLIST_NEXT_ALL,
                                  wxLIST_STATE_SELECTED);
   if (item == -1 || item + 2 >= mList->GetItemCount()) {
      return;
   }

   mBatchCommands.AddToChain(mBatchCommands.GetCommand(item),
                             mBatchCommands.GetParams(item),
                             item + 2);
   mBatchCommands.DeleteFromChain(item);

   mChanged = true;

   mSelectedCommand = item + 1;

   PopulateList();
}

/// Select the empty Command chain.
void EditChainsDialog::OnDefaults(wxCommandEvent &event)
{
   mBatchCommands.RestoreChain(mActiveChain);

   mChanged = true;

   PopulateList();
}

/// Send changed values back to Prefs, and update Audacity.
void EditChainsDialog::OnOK(wxCommandEvent &event)
{
   gPrefs->Write(wxT("/Batch/ActiveChain"), mActiveChain);

   if (mChanged) {
      if (!mBatchCommands.WriteChain(mActiveChain)) {
         return;
      }
   }

   EndModal(true);
}

///
void EditChainsDialog::OnCancel(wxCommandEvent &event)
{
   if (!ChangeOK()) {
      return;
   }

   EndModal(false);
}

///
void EditChainsDialog::OnKeyDown(wxKeyEvent &event)
{
   if (event.GetKeyCode() == WXK_DELETE) {
      printf("hello\n");
   }

   event.Skip();
}

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: TBD


