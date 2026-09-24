using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Text;

public static class GalaxyDiagnosticSetup {
    // Deliberately no arbitrary device/class-property entry point.
    const string Target = @"HID\WCOM016C&Col01\5&a6b5543&0&0000";
    // SetupAPI.h: SPDRP_UPPERFILTERS, not SPDRP_BUSTYPEGUID (0x13).
    const uint UpperFiltersProperty = 0x11;
    [StructLayout(LayoutKind.Sequential)] struct DeviceInfo {
        public uint Size; public Guid ClassGuid; public uint DevInst; public UIntPtr Reserved;
    }
    [DllImport("setupapi.dll", SetLastError=true)] static extern IntPtr SetupDiCreateDeviceInfoList(IntPtr cls, IntPtr parent);
    [DllImport("setupapi.dll", CharSet=CharSet.Unicode, SetLastError=true)] static extern bool SetupDiOpenDeviceInfoW(IntPtr set,string id,IntPtr parent,uint flags,ref DeviceInfo data);
    [DllImport("setupapi.dll", SetLastError=true)] static extern bool SetupDiGetDeviceRegistryPropertyW(IntPtr set,ref DeviceInfo data,uint property,out uint type,byte[] buffer,uint size,out uint required);
    [DllImport("setupapi.dll", SetLastError=true)] static extern bool SetupDiSetDeviceRegistryPropertyW(IntPtr set,ref DeviceInfo data,uint property,byte[] buffer,uint size);
    [DllImport("setupapi.dll")] static extern bool SetupDiDestroyDeviceInfoList(IntPtr set);
    [DllImport("newdev.dll", CharSet=CharSet.Unicode, SetLastError=true)] static extern bool DiInstallDriverW(IntPtr parent,string inf,uint flags,out bool reboot);
    [DllImport("newdev.dll", CharSet=CharSet.Unicode, SetLastError=true)] static extern bool DiUninstallDriverW(IntPtr parent,string inf,uint flags,out bool reboot);
    public static bool Install(string inf) {bool reboot;if(!DiInstallDriverW(IntPtr.Zero,inf,0,out reboot))throw new Win32Exception();return reboot;}
    public static bool Uninstall(string inf) {bool reboot;if(!DiUninstallDriverW(IntPtr.Zero,inf,0,out reboot))throw new Win32Exception();return reboot;}
    static string ReadUpperFilters(IntPtr set,ref DeviceInfo device) {
        var buffer=new byte[4096];uint type,needed;
        if(!SetupDiGetDeviceRegistryPropertyW(set,ref device,UpperFiltersProperty,out type,buffer,(uint)buffer.Length,out needed))throw new Win32Exception();
        if(type!=7 || needed>buffer.Length || needed<4 || needed%2!=0)throw new InvalidOperationException("UpperFilters inválido.");
        string value=Encoding.Unicode.GetString(buffer,0,(int)needed);
        if(!value.EndsWith("\0\0",StringComparison.Ordinal))throw new InvalidOperationException("UpperFilters sem terminador.");
        return value.TrimEnd('\0');
    }
    // Read-only probe exercises the same property and decoder used before writes.
    public static string[] GetUpperFilters() {
        var set=SetupDiCreateDeviceInfoList(IntPtr.Zero,IntPtr.Zero);
        if(set==new IntPtr(-1))throw new Win32Exception();
        try {
            var device=new DeviceInfo();device.Size=(uint)Marshal.SizeOf(typeof(DeviceInfo));
            if(!SetupDiOpenDeviceInfoW(set,Target,IntPtr.Zero,0,ref device))throw new Win32Exception();
            return ReadUpperFilters(set,ref device).Split('\0');
        } finally {SetupDiDestroyDeviceInfoList(set);}
    }
    public static void SetDiagnosticAttachment(bool attach) {
        string[] original={"PenS2Helper"};
        string[] diagnostic={"GalaxyPenDiagB","PenS2Helper","GalaxyPenDiagA"};
        var set=SetupDiCreateDeviceInfoList(IntPtr.Zero,IntPtr.Zero);
        if(set==new IntPtr(-1))throw new Win32Exception();
        try {
            var device=new DeviceInfo();device.Size=(uint)Marshal.SizeOf(typeof(DeviceInfo));
            if(!SetupDiOpenDeviceInfoW(set,Target,IntPtr.Zero,0,ref device))throw new Win32Exception();
            string current=ReadUpperFilters(set,ref device);
            string oldValue=string.Join("\0",original),newValue=string.Join("\0",diagnostic);
            string expected=attach?oldValue:newValue,desired=attach?newValue:oldValue;
            if(current==desired)return;
            if(current!=expected)throw new InvalidOperationException("Filtros inesperados: nenhuma lista foi sobrescrita.");
            byte[] encoded=Encoding.Unicode.GetBytes(desired+"\0\0");
            if(!SetupDiSetDeviceRegistryPropertyW(set,ref device,UpperFiltersProperty,encoded,(uint)encoded.Length))throw new Win32Exception();
        } finally {SetupDiDestroyDeviceInfoList(set);}
    }
}
