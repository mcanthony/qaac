const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.55";
#else
    return "2.55";
#endif
}
