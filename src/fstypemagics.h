#ifndef FSTYPEMAGICS_H
#define FSTYPEMAGICS_H

/* from "man statfs" */

/* remote */
#define  CODA_SUPER_MAGIC      0x73757245
#define  CIFS_MAGIC_NUMBER     0xFF534D42
#define  NCP_SUPER_MAGIC       0x564c
#define  NFS_SUPER_MAGIC       0x6969
#define  SMB_SUPER_MAGIC       0x517B
#define  VXFS_SUPER_MAGIC      0xa501FCF5

static_var const long remote_magic[] = {
    CODA_SUPER_MAGIC,
    CIFS_MAGIC_NUMBER,
    NCP_SUPER_MAGIC,
    NFS_SUPER_MAGIC,
    SMB_SUPER_MAGIC,
    VXFS_SUPER_MAGIC,
    0
};
static_var const char *const remote_magic_name[] = {
    "CODA",
    "CIFS",
    "NCP",
    "NFS",
    "SMB",
    "VXFS",
    NULL
};

/* local */
#define  ADFS_SUPER_MAGIC      0xadf5   /* advanced disc filing system, acorn and riscOS */
#define  AFFS_SUPER_MAGIC      0xADFF   /* amiga fast file system */
#define  BEFS_SUPER_MAGIC      0x42465331
#define  BFS_MAGIC             0x1BADFACE
#define  COH_SUPER_MAGIC       0x012FF7B7       /* Coherent filesystem */
#define  CRAMFS_MAGIC          0x28cd3d45
#define  DEVFS_SUPER_MAGIC     0x1373
#define  EFS_SUPER_MAGIC       0x00414A53
#define  EXT_SUPER_MAGIC       0x137D
#define  EXT2_OLD_SUPER_MAGIC  0xEF51
#define  EXT2_SUPER_MAGIC      0xEF53
#define  EXT3_SUPER_MAGIC      0xEF53
#define  HFS_SUPER_MAGIC       0x4244
#define  HPFS_SUPER_MAGIC      0xF995E849
#define  HUGETLBFS_MAGIC       0x958458f6
#define  ISOFS_SUPER_MAGIC     0x9660
#define  JFFS2_SUPER_MAGIC     0x72b6
#define  JFS_SUPER_MAGIC       0x3153464a
#define  MINIX_SUPER_MAGIC     0x137F   /* orig. minix */
#define  MINIX_SUPER_MAGIC2    0x138F   /* 30 char minix */
#define  MINIX2_SUPER_MAGIC    0x2468   /* minix V2 */
#define  MINIX2_SUPER_MAGIC2   0x2478   /* minix V2, 30 char names */
#define  MSDOS_SUPER_MAGIC     0x4d44
#define  NTFS_SB_MAGIC         0x5346544e
#define  OPENPROM_SUPER_MAGIC  0x9fa1
#define  PROC_SUPER_MAGIC      0x9fa0
#define  QNX4_SUPER_MAGIC      0x002f
#define  REISERFS_SUPER_MAGIC  0x52654973
#define  ROMFS_MAGIC           0x7275
#define  SYSV2_SUPER_MAGIC     0x012FF7B6
#define  SYSV4_SUPER_MAGIC     0x012FF7B5
#define  TMPFS_MAGIC           0x01021994
#define  UDF_SUPER_MAGIC       0x15013346
#define  UFS_MAGIC             0x00011954
#define  USBDEVICE_SUPER_MAGIC 0x9fa2
#define  XENIX_SUPER_MAGIC     0x012FF7B4
#define  XFS_SUPER_MAGIC       0x58465342
#define  _XIAFS_SUPER_MAGIC    0x012FD16D

static_var const long local_magic[] = {
    ADFS_SUPER_MAGIC,
    AFFS_SUPER_MAGIC,
    BEFS_SUPER_MAGIC,
    BFS_MAGIC,
    COH_SUPER_MAGIC,
    CRAMFS_MAGIC,
    DEVFS_SUPER_MAGIC,
    EFS_SUPER_MAGIC,
    EXT_SUPER_MAGIC,
    EXT2_OLD_SUPER_MAGIC,
    EXT2_SUPER_MAGIC,
    EXT3_SUPER_MAGIC,
    HFS_SUPER_MAGIC,
    HPFS_SUPER_MAGIC,
    HUGETLBFS_MAGIC,
    ISOFS_SUPER_MAGIC,
    JFFS2_SUPER_MAGIC,
    JFS_SUPER_MAGIC,
    MINIX_SUPER_MAGIC,
    MINIX_SUPER_MAGIC2,
    MINIX2_SUPER_MAGIC,
    MINIX2_SUPER_MAGIC2,
    MSDOS_SUPER_MAGIC,
    NTFS_SB_MAGIC,
    OPENPROM_SUPER_MAGIC,
    PROC_SUPER_MAGIC,
    QNX4_SUPER_MAGIC,
    REISERFS_SUPER_MAGIC,
    ROMFS_MAGIC,
    SYSV2_SUPER_MAGIC,
    SYSV4_SUPER_MAGIC,
    TMPFS_MAGIC,
    UDF_SUPER_MAGIC,
    UFS_MAGIC,
    USBDEVICE_SUPER_MAGIC,
    XENIX_SUPER_MAGIC,
    XFS_SUPER_MAGIC,
    _XIAFS_SUPER_MAGIC,
    0
};
static_var const char *const local_magic_name[] = {
    "ADFS",
    "AFFS",
    "BEFS",
    "BFS",
    "COH",
    "CRAMFS",
    "DEVFS",
    "EFS",
    "EXT",
    "EXT2",
    "EXT2",
    "EXT3",
    "HFS",
    "HPFS",
    "HUGETLBFS",
    "ISOFS",
    "JFFS2",
    "JFS",
    "MINIX",
    "MINIX",
    "MINIX2",
    "MINIX2",
    "MSDOS",
    "NTFS_SB",
    "OPENPROM",
    "PROC",
    "QNX4",
    "REISERFS",
    "ROMFS",
    "SYSV2",
    "SYSV4",
    "TMPFS",
    "UDF",
    "UFS",
    "USBDEVICE",
    "XENIX",
    "XFS",
    "XIAFS",
    NULL
};

#endif /* FSTYPEMAGICS_H */
