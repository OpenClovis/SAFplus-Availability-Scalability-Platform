package com.clovis.cw.licensing;

/**
 * 
 * @author Pushparaj
 * Class for user information
 */
public class UserInfo {
	private String _loginName = "", _password = "";
	private boolean _isVerifiedUser;
	/**
	 * Creates UserInfo instance with empty values
	 */
	public UserInfo() {
		_loginName = "";
		_password = "";
	}
	/**
	 * Creates UserInfo instance with loginName and password
	 * @param loginName Login Name
	 * @param password  Password
	 */
	public UserInfo(String loginName, String password){
		_loginName = loginName;
		_password = password;
	}
	/**
	 * Set log-in name
	 * @param login log-in name
	 */
	public void setLoginName(String login){
		_loginName = login;
	}
	/**
	 * Returns log-in name
	 * @return _loginName
	 */
	public String getLoginName() {
		return _loginName;
	}
	/**
	 * Set password
	 * @param pwd password
	 */
	public void setPassword(String pwd){
		_password = pwd;
	}
	/**
	 * Returns password
	 * @return _password
	 */
	public String getPassword() {
		return _password;		
	}
	/**
	 * Sets user verification status
	 * @param verified user verification status
	 */
	public void setUserVerification(boolean verified) {
		_isVerifiedUser = verified;
	}
	/**
	 * Returns user verification status
	 * @return user verification
	 */
	public boolean isVerifiedUser() {
		return _isVerifiedUser;
	}
}