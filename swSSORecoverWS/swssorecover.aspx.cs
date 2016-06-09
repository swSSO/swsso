//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2016 - Sylvain WERDEFROY
//
//							 http://www.swsso.fr
//                   
//                             sylvain@swsso.fr
//
//-----------------------------------------------------------------------------
// 
//  This file is part of swSSO.
//  
//  swSSO is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  swSSO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with swSSO.  If not, see <http://www.gnu.org/licenses/>.
// 
//-----------------------------------------------------------------------------
// Web Service de resynchronisation de mot de passe 
// Fourni à titre d'exemple d'utilisation de la DLL swSSORecoverDll
//-----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Threading;
using System.Security.Principal;
using Newtonsoft.Json;

public partial class _Default : System.Web.UI.Page
{
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool SetDllDirectory(string lpPathName);
    
    [DllImport(@"swSSORecoverDll.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int RecoveryGetResponse(String strChallenge, String StrDomainUserName, StringBuilder strResponse, int iMaxCount);

    [DllImport("advapi32.dll", CharSet = CharSet.Auto)]
    public static extern int LogonUser(String lpszUserName,
                                  String lpszDomain,
                                  String lpszPassword,
                                  int dwLogonType,
                                  int dwLogonProvider,
                                  ref IntPtr phToken);
    [DllImport("advapi32.dll", CharSet = System.Runtime.InteropServices.CharSet.Auto, SetLastError = true)]
    public extern static int DuplicateToken(IntPtr hToken,
                                  int impersonationLevel,
                                  ref IntPtr hNewToken);
    public const int LOGON32_LOGON_INTERACTIVE = 2;
    public const int LOGON32_PROVIDER_DEFAULT = 0;

    public class swSSOChallenge
    {
        public string challenge;
    }
    public class swSSOResponse
    {
        public string response;
    }

    //-----------------------------------------------------------------------------
    // Classe de déchiffrement de mot de passe chiffré avec DPAPI
    //-----------------------------------------------------------------------------
    public class swSSOPwdProtect
    {
        public swSSOPwdProtect()
        {
        }
        #region InteropServices
        //-----------------------------------------------------------------------------
        // Déclaration des fonctions, structures et constantes DPAPI pour le chiffrement / déchiffrement du mot de passe
        //-----------------------------------------------------------------------------
        [DllImport("crypt32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        private static extern bool CryptProtectData(ref DATA_BLOB pClearBlob, IntPtr noDescription, IntPtr noEntropy, IntPtr pReserved, IntPtr noPromptStruct, int dwFlags, ref DATA_BLOB pEncryptedBlob);

        [DllImport("crypt32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        private static extern bool CryptUnprotectData(ref DATA_BLOB pEncryptedBlob, IntPtr noDescription, IntPtr noEntropy, IntPtr pReserved, IntPtr noPromptStruct, int dwFlags, ref DATA_BLOB pClearBlob);

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        internal struct DATA_BLOB
        {
            public int cbData;
            public IntPtr pbData;
        }
        private const int CRYPTPROTECT_UI_FORBIDDEN = 0x1;
        private const int CRYPTPROTECT_LOCAL_MACHINE = 0x4;
        #endregion

        //-----------------------------------------------------------------------------
        // Decrypt
        //-----------------------------------------------------------------------------
        public static string Decrypt(string strEncrypted)
        {
            string strClear = null;
            DATA_BLOB clearBlob = new DATA_BLOB();
            clearBlob.cbData = 0;
            clearBlob.pbData = IntPtr.Zero;
            DATA_BLOB encryptedBlob = new DATA_BLOB();
            encryptedBlob.cbData = 0;
            encryptedBlob.pbData = IntPtr.Zero;

            try
            {
                int len = strEncrypted.Length;
                byte[] encryptedBytes = new byte[len / 2];
                for (int i = 0; i < len; i += 2)
                encryptedBytes[i / 2] = Convert.ToByte(strEncrypted.Substring(i, 2), 16);
                    
                encryptedBlob.cbData = encryptedBytes.Length;
                encryptedBlob.pbData = Marshal.AllocHGlobal(encryptedBytes.Length);
                Marshal.Copy(encryptedBytes, 0, encryptedBlob.pbData, encryptedBytes.Length);
                if (CryptUnprotectData(ref encryptedBlob, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, CRYPTPROTECT_UI_FORBIDDEN, ref clearBlob))
                {
                    byte[] clearBytes = new byte[clearBlob.cbData];
                    Marshal.Copy(clearBlob.pbData, clearBytes, 0, clearBlob.cbData);
                    strClear = Encoding.UTF8.GetString(clearBytes);
                }
            }
            catch (Exception ex)
            {
                string strex = ex.ToString();
            }
            finally
            {
                if (encryptedBlob.pbData != IntPtr.Zero) Marshal.FreeHGlobal(encryptedBlob.pbData);
            }
            return strClear;
        }
    }

    //-----------------------------------------------------------------------------
    // Code principal : traite le challenge et retourne la réponse
    //-----------------------------------------------------------------------------
    protected void Page_Load(object sender, EventArgs e)
    {
        //
        // initialisations
        //
        String strTrace = "";
        StringBuilder strResponse = new StringBuilder(256);
        WindowsIdentity tempWindowsIdentity;
        IntPtr token = IntPtr.Zero;
        IntPtr tokenDuplicate = IntPtr.Zero;
        WindowsImpersonationContext impersonationContext;

        //
        // lecture de la configuration dans web.config (infos du compte de service + debug or not debug)
        //
        string strUser = System.Configuration.ConfigurationManager.AppSettings["user"].ToString();
        string strDomain = System.Configuration.ConfigurationManager.AppSettings["domain"].ToString();
        string strEncryptedPassword = System.Configuration.ConfigurationManager.AppSettings["password"].ToString();
        string strConfigDebug = System.Configuration.ConfigurationManager.AppSettings["debug"].ToString();
        string strRecoverDllPath = System.Configuration.ConfigurationManager.AppSettings["dllpath"].ToString();
        strTrace += "strUser=" + strUser + "<br/>";
        strTrace += "strDomain=" + strDomain + "<br/>";
        strTrace += "strEncryptedPassword=" + strEncryptedPassword + "<br/>";
        strTrace += "strRecoverDllPath=" + strRecoverDllPath + "<br/>";

        // debut test d'appel de la DLL -- pour debug
        // string testChallenge = "testchallenge";
        // string testUser = "testuser";
        // SetDllDirectory(strRecoverDllPath);
        // RecoveryGetResponse(testChallenge, testUser, strResponse, strResponse.Capacity);
        // Response.StatusCode = 200;
        // Response.Write("fintest");
        // Response.End();
        // return;
        // fin test d'appel de la DLL -- pour debug

        //
        // déchiffrement du mot de passe du compte de service
        //
        string strPassword = swSSOPwdProtect.Decrypt(strEncryptedPassword);
        strTrace += "strPassword=" + strPassword + "<br/>";

        //
        // impersonation avec le compte de service
        //
        strTrace += "AVANT IMPERSONATION : process=" + System.Security.Principal.WindowsIdentity.GetCurrent().Name + "<br/>";
        if (LogonUser(strUser,strDomain, strPassword, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, ref token) != 0)
        {
            if (DuplicateToken(token, 2, ref tokenDuplicate) != 0)
            {
                tempWindowsIdentity = new WindowsIdentity(tokenDuplicate);
                impersonationContext = tempWindowsIdentity.Impersonate();
            }
        }
        strTrace += "APRES IMPERSONATION : process=" + System.Security.Principal.WindowsIdentity.GetCurrent().Name + "<br/>";
        if (Request.LogonUserIdentity.IsAuthenticated) strTrace += "authenticated!<br/>";
        strTrace += "name=" + HttpContext.Current.Request.LogonUserIdentity.Name + "<br/>";

        //
        // extraction du challenge de la requete au format JSON
        //
        strTrace += "Taille donnees postees =" + Request.TotalBytes + "<br/>";
        string strData="";
        if (Request.TotalBytes>0)
        {
            strData+=Encoding.Default.GetString(Request.BinaryRead(Request.TotalBytes));
        }
        //
        // désérialisation du JSON
        //
        swSSOChallenge objChallenge = JsonConvert.DeserializeObject<swSSOChallenge>(strData);
        strTrace += "objChallenge.challenge=" + objChallenge.challenge;
        
        //
        // calcul de la réponse
        //
        SetDllDirectory(strRecoverDllPath);
        strTrace += "RecoveryGetResponse=" + RecoveryGetResponse(objChallenge.challenge,
            HttpContext.Current.Request.LogonUserIdentity.Name,
            strResponse,
            strResponse.Capacity)+"<br/>";
        strTrace += "strResponse=" + strResponse + "<br/>";

        //
        // sérialisation de la réponse au format JSON
        //
        swSSOResponse objResponse = new swSSOResponse();
        objResponse.response=strResponse.ToString();
        strTrace += "objResponse.response=" + objResponse.response;

        //
        // envoi de la réponse
        //
        if (strConfigDebug == "1") Response.Write(strTrace + "---------------------------------------<br/>");
        Response.StatusCode = 200;
        Response.Write(JsonConvert.SerializeObject(objResponse));
        Response.End();
    }
}
