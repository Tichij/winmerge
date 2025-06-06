/** 
 * @file  LoadSaveCodepageDlg.cpp
 *
 * @brief Implementation of the dialog used to select codepages
 */

#include "stdafx.h"
#include "LoadSaveCodepageDlg.h"
#include "resource.h"
#include "ExConverter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoadSaveCodepageDlg dialog


CLoadSaveCodepageDlg::CLoadSaveCodepageDlg(int nFiles, CWnd* pParent /*= nullptr*/)
: CTrDialog(CLoadSaveCodepageDlg::IDD, pParent)
, m_bAffectsLeft(true)
, m_bAffectsMiddle(true)
, m_bAffectsRight(true)
, m_bLoadSaveSameCodepage(true)
// String m_sAffectsLeftString
// String m_sAffectsRightString
, m_bSaveCodepageBOM(false)
, m_nLoadCodepage(-1)
, m_nSaveCodepage(-1)
, m_bEnableSaveCodepage(false)
, m_nFiles(nFiles)
{
	//{{AFX_DATA_INIT(CLoadSaveCodepageDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CLoadSaveCodepageDlg::SetLeftRightAffectStrings(const String & sAffectsLeft, const String & sAffectsMiddle, const String & sAffectsRight)
{
	m_sAffectsLeftString = sAffectsLeft;
	m_sAffectsMiddleString = sAffectsMiddle;
	m_sAffectsRightString = sAffectsRight;
}

void CLoadSaveCodepageDlg::DoDataExchange(CDataExchange* pDX)
{
	CTrDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoadSaveCodepageDlg)
	DDX_Check(pDX, IDC_AFFECTS_LEFT_BTN, m_bAffectsLeft);
	DDX_Check(pDX, IDC_AFFECTS_MIDDLE_BTN, m_bAffectsMiddle);
	DDX_Check(pDX, IDC_AFFECTS_RIGHT_BTN, m_bAffectsRight);
	DDX_Check(pDX, IDC_LOAD_SAVE_SAME_CODEPAGE, m_bLoadSaveSameCodepage);
	DDX_Control(pDX, IDC_AFFECTS_LEFT_BTN, m_AffectsLeftBtn);
	DDX_Control(pDX, IDC_AFFECTS_MIDDLE_BTN, m_AffectsMiddleBtn);
	DDX_Control(pDX, IDC_AFFECTS_RIGHT_BTN, m_AffectsRightBtn);
	DDX_Text(pDX, IDC_LOAD_CODEPAGE_TEXTBOX, m_nLoadCodepage);
	DDX_Text(pDX, IDC_SAVE_CODEPAGE_TEXTBOX, m_nSaveCodepage);
	DDX_Check(pDX, IDC_SAVE_CODEPAGE_BOM, m_bSaveCodepageBOM);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoadSaveCodepageDlg, CTrDialog)
	//{{AFX_MSG_MAP(CLoadSaveCodepageDlg)
	ON_BN_CLICKED(IDC_AFFECTS_LEFT_BTN, OnAffectsLeftBtnClicked)
	ON_BN_CLICKED(IDC_AFFECTS_MIDDLE_BTN, OnAffectsMiddleBtnClicked)
	ON_BN_CLICKED(IDC_AFFECTS_RIGHT_BTN, OnAffectsRightBtnClicked)
	ON_BN_CLICKED(IDC_LOAD_SAVE_SAME_CODEPAGE, OnLoadSaveSameCodepage)
	ON_MESSAGE(WM_APP, OnLoadCodepages)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoadSaveCodepageDlg message handlers

/**
 * @brief Handler for WM_INITDIALOG; conventional location to initialize controls
 * At this point dialog and control windows exist
 */
BOOL CLoadSaveCodepageDlg::OnInitDialog() 
{
	CTrDialog::OnInitDialog();

	CenterWindow();

	SetDlgItemText(IDC_LEFT_FILES_LABEL, m_sAffectsLeftString);
	SetDlgItemText(IDC_MIDDLE_FILES_LABEL, m_sAffectsMiddleString);
	SetDlgItemText(IDC_RIGHT_FILES_LABEL, m_sAffectsRightString);

	UpdateSaveGroup();

	if (m_nFiles < 3)
		EnableDlgItem(IDC_AFFECTS_MIDDLE_BTN, false);

	m_asyncCodepagesLoader = Concurrent::CreateTask([hwnd = m_hWnd] {
			std::vector<CodePageInfo> cpi;
			IExconverter *pexconv = Exconverter::getInstance();
			if (pexconv != nullptr)
				cpi = pexconv->enumCodePages();
			const static int ManualAddTypeList[] = { 437, 850, 858, 860, 863, 861, 1200, 1201, 65000 };
			for (int codepage : ManualAddTypeList)
			{
				const int cp = codepage;
				auto it = std::find_if(cpi.begin(), cpi.end(),
					[&](const CodePageInfo& info) { return info.codepage == cp; });
				if (it == cpi.end())
				{
					CodePageInfo cpii{};
					pexconv->getCodepageDescription(cp, cpii.desc);
					cpii.codepage = cp;
					cpi.push_back(cpii);
				}
			}
			std::sort(cpi.begin(), cpi.end(),
				[](const CodePageInfo& a, const CodePageInfo& b) { return a.codepage < b.codepage; });
			::PostMessage(hwnd, WM_APP, 0, 0);
			return cpi;
		});

	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/**
 * @brief If user unchecks left, then check right (to ensure never have nothing checked)
 */
void CLoadSaveCodepageDlg::OnAffectsLeftBtnClicked()
{
	if (m_AffectsLeftBtn.GetCheck() != BST_CHECKED)
	{
		if (m_nFiles < 3)
			m_AffectsRightBtn.SetCheck(BST_CHECKED);
		else
		{
			if (m_AffectsMiddleBtn.GetCheck() != BST_CHECKED)
			{
				if (m_AffectsRightBtn.GetCheck() != BST_CHECKED)
					m_AffectsRightBtn.SetCheck(BST_CHECKED);
			}
		}
	}
}

/**
 * @brief If user unchecks middle, then check right (to ensure never have nothing checked)
 */
void CLoadSaveCodepageDlg::OnAffectsMiddleBtnClicked()
{
	if (m_AffectsMiddleBtn.GetCheck() != BST_CHECKED)
	{
		if (m_AffectsLeftBtn.GetCheck() != BST_CHECKED)
		{
			if (m_AffectsRightBtn.GetCheck() != BST_CHECKED)
				m_AffectsLeftBtn.SetCheck(BST_CHECKED);
		}
	}
}

/**
 * @brief If user unchecks right, then check left (to ensure never have nothing checked)
 */
void CLoadSaveCodepageDlg::OnAffectsRightBtnClicked()
{
	if (m_AffectsRightBtn.GetCheck() != BST_CHECKED)
	{
		if (m_nFiles < 3)
			m_AffectsLeftBtn.SetCheck(BST_CHECKED);
		else
		{
			if (m_AffectsMiddleBtn.GetCheck() != BST_CHECKED)
			{
				if (m_AffectsLeftBtn.GetCheck() != BST_CHECKED)
					m_AffectsLeftBtn.SetCheck(BST_CHECKED);
			}
		}
	}
}

/**
 * @brief Disable save group if save codepage slaved to load codepage
 */
void CLoadSaveCodepageDlg::OnLoadSaveSameCodepage()
{
	UpdateSaveGroup();
}

LRESULT CLoadSaveCodepageDlg::OnLoadCodepages(WPARAM, LPARAM)
{
	CComboBox combol, combos;
	combol.Attach(::GetDlgItem(m_hWnd, IDC_LOAD_CODEPAGE_TEXTBOX));
	combos.Attach(::GetDlgItem(m_hWnd, IDC_SAVE_CODEPAGE_TEXTBOX));
	m_cpList = m_asyncCodepagesLoader.Get();
	for (size_t i = 0, j = 0; i < m_cpList.size(); i++)
	{
		String desc = strutils::format(_T("%05d - %s"), m_cpList[i].codepage, m_cpList[i].desc);
		combol.AddString(desc.c_str());
		combos.AddString(desc.c_str());
		if (m_cpList[i].codepage == m_nLoadCodepage)
		{
			combol.SetCurSel(static_cast<int>(j));
			combos.SetCurSel(static_cast<int>(j));
		}
		j++;
	}
	combol.SetFocus();
	combol.Detach();
	combos.Detach();
	UpdateSaveGroup();
	return 0;
}

/**
 * @brief Disable save group if save codepage slaved to load codepage
 */
void CLoadSaveCodepageDlg::UpdateSaveGroup()
{
	UpdateDataFromWindow();
	EnableDlgItem(IDC_LOAD_CODEPAGE_TEXTBOX, m_cpList.size() > 0);
	if (!m_bEnableSaveCodepage)
		EnableDlgItem(IDC_LOAD_SAVE_SAME_CODEPAGE, false);
	bool EnableSave = m_bEnableSaveCodepage && !m_bLoadSaveSameCodepage;
	EnableDlgItem(IDC_SAVE_CODEPAGE_TEXTBOX, EnableSave && m_cpList.size() > 0);
	EnableDlgItem(IDC_SAVE_CODEPAGE_BOM, EnableSave);
}

/**
 * @brief User pressed Ok, ensure data members set correctly
 */
void CLoadSaveCodepageDlg::OnOK ()
{
	CTrDialog::OnOK();
	if (m_bLoadSaveSameCodepage)
	{
		m_nSaveCodepage = m_nLoadCodepage;
	}
}
