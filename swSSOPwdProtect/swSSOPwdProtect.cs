using System;
using System.Collections.Generic;
using System.Web;
using System.Runtime.InteropServices;
using System.Text;

public class swSSOPwdProtect
{
    public swSSOPwdProtect()
	{
		
	}
#region InteropServices
    // déclaration des fonction, structures et constantes DAPI pour le chiffrement / déchiffrement du mot de passe
    [DllImport("crypt32.dll",SetLastError = true,CharSet = CharSet.Auto)]
    private static extern bool CryptProtectData(ref DATA_BLOB pClearBlob, IntPtr noDescription, IntPtr noEntropy, IntPtr pReserved, IntPtr noPromptStruct, int dwFlags, ref DATA_BLOB pEncryptedBlob);
    
    [DllImport("crypt32.dll",SetLastError = true,CharSet = CharSet.Auto)]
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

    // chiffrement de la chaine strClear
    public static string Encrypt(string strClear)
    {
        string strEncrypted=null;
        DATA_BLOB clearBlob = new DATA_BLOB();
        clearBlob.cbData = 0;
        clearBlob.pbData = IntPtr.Zero;
        DATA_BLOB encryptedBlob = new DATA_BLOB();
        encryptedBlob.cbData = 0;
        encryptedBlob.pbData = IntPtr.Zero;
       
        try
        {
            byte[] clearBytes = Encoding.UTF8.GetBytes(strClear);
            clearBlob.cbData = clearBytes.Length;
            clearBlob.pbData = Marshal.AllocHGlobal(clearBytes.Length);
            Marshal.Copy(clearBytes, 0, clearBlob.pbData, clearBytes.Length);
            if (CryptProtectData(ref clearBlob, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, CRYPTPROTECT_LOCAL_MACHINE | CRYPTPROTECT_UI_FORBIDDEN, ref encryptedBlob))
            {
                byte[] encryptedBytes = new byte[encryptedBlob.cbData];
                Marshal.Copy(encryptedBlob.pbData, encryptedBytes, 0, encryptedBlob.cbData);
                StringBuilder hex = new StringBuilder(encryptedBytes.Length * 2);
				foreach (byte b in encryptedBytes)	hex.AppendFormat("{0:X2}", b);
				strEncrypted=hex.ToString();
            }
        }
        catch (Exception ex)
        {

        }
        finally 
        {
            if (clearBlob.pbData != IntPtr.Zero) Marshal.FreeHGlobal(clearBlob.pbData);
        }
        return strEncrypted;
    }

    // déchiffrement de la chaine strEncrypted (encodée en base 64)
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
            int NumberChars = strEncrypted.Length;
			byte[] encryptedBytes = new byte[NumberChars / 2];
			for (int i = 0; i < NumberChars; i += 2) encryptedBytes[i / 2] = Convert.ToByte(strEncrypted.Substring(i, 2), 16);
			
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

        }
        finally
        {
            if (encryptedBlob.pbData != IntPtr.Zero) Marshal.FreeHGlobal(encryptedBlob.pbData);
        }
        return strClear;
    }
}